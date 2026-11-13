# Compression Header Format {#format}

This page defines the binary header written before each compressed data
payload. The header contains the information needed to interpret the payload.

## Parsing a Header

Use [`cmp_hdr_deserialize()`](@ref cmp_hdr_deserialize) to parse a header from a
compressed data buffer into [`struct cmp_hdr`](@ref cmp_hdr).
## Header Structure

The header is 24 bytes (`CMP_HDR_SIZE`). All multi-byte integers use big-endian
byte order. Offsets are relative to the start of the header.

```text
 byte      0       1       2       3       4       5       6       7
      +---------------+-----------------------+-----------------------+
      | Version       | Compressed Size       | Original Size         |
      | 16 bits       | 24 bits               | 24 bits               |
      +---------------+-----------------------+-----------------------+

 byte      8       9      10      11      12      13      14      15
      +-------------------------------+-------------------------------+
      | Checksum                      | Identifier                    |
      | 32 bits                       | 32 bits                       |
      +-------------------------------+-------------------------------+

 byte     16      17      18      19      20      21      22      23
      +-------+-------+---------------+-----------------------+-------+
      | Seq.  | PED   | Encoder Param | Encoder Outlier       | PP    |
      | 8 bit | 8 bit | 16 bits       | 24 bits               | 8 bit |
      +-------+-------+---------------+-----------------------+-------+
      | Data payload (variable size)                                  |
      |                              ...                              |
      +---------------------------------------------------------------+
```

`PED` contains Preprocessing, Encoder Type, and Original DType. `PP` is
Preprocess Param.

| Offsets | Field                                              | Bits | Description                                             |
| ------: | -------------------------------------------------- | ---: | ------------------------------------------------------- |
|       0 | [Version](@ref cmp_hdr::version)                   |   16 | Version of the software that produced the header        |
|       2 | [Compressed Size](@ref cmp_hdr::compressed_size)   |   24 | Total size of the header and payload in bytes           |
|       5 | [Original Size](@ref cmp_hdr::original_size)       |   24 | Size of the original packed data in bytes               |
|       8 | [Checksum](@ref cmp_hdr::checksum)                 |   32 | Checksum of the original data, or zero if disabled      |
|      12 | [Identifier](@ref cmp_hdr::identifier)             |   32 | Identifier shared by passes in one compression sequence |
|      16 | [Sequence Number](@ref cmp_hdr::sequence_number)   |    8 | Pass position within the sequence, starting at zero     |
|      17 | [Preprocessing](@ref cmp_hdr::preprocessing)       |    3 | Preprocessing method used for the payload               |
|      17 | [Encoder Type](@ref cmp_hdr::encoder_type)         |    2 | Encoder used for the payload                            |
|      17 | [Original DType](@ref cmp_hdr::original_dtype)     |    3 | Data type of the original input                         |
|      18 | [Encoder Param](@ref cmp_hdr::encoder_param)       |   16 | Encoder-specific parameter, or zero if unused           |
|      20 | [Encoder Outlier](@ref cmp_hdr::encoder_outlier)   |   24 | Effective Golomb outlier threshold, or zero if unused   |
|      23 | [Preprocess Param](@ref cmp_hdr::preprocess_param) |    8 | Preprocessing-specific parameter                        |

## Field Values

See the API definitions of [`cmp_preprocessing`](@ref cmp_preprocessing),
[`cmp_encoder_type`](@ref cmp_encoder_type), and [`cmp_type`](@ref cmp_type) for
the encoded field values.

## Implementation Notes

**Version:** The version is encoded as `major * 10000 + minor * 100 + release`,
see [`CMP_VERSION_NUMBER`](@ref CMP_VERSION_NUMBER)

**Size limits:** Compressed Size and Original Size have a maximum value of
`2^24 - 1` bytes. Compressed Size includes the 24-byte header.

**Sequence handling:** A primary pass has Sequence Number zero. Secondary
passes increment it while retaining the same Identifier. A context reset,
completed sequence, or uncompressed fallback assigns a new Identifier and
starts again at zero.

**Preprocess Param:** For `CMP_PREPROCESS_MODEL`, this field contains
[`model_rate`](@ref cmp_params::model_rate). For all other methods, it contains
[`secondary_iterations`](@ref cmp_params::secondary_iterations).

**Uncompressed data:** If Preprocessing is `CMP_PREPROCESS_NONE` and Encoder
Type is `CMP_ENCODER_UNCOMPRESSED`, the payload is uncompressed. `CMP_I16`,
`CMP_U16`, and `CMP_I16_IN_I32` samples are stored as big-endian 16-bit values.
`CMP_RAW12` is copied in its packed byte representation. Compressed Size is
Header Size plus Original Size.

**Uncompressed with preprocessing:** If preprocessing is combined with
`CMP_ENCODER_UNCOMPRESSED`, each transformed value is stored as a big-endian
16-bit value. This testing-oriented combination may produce a payload larger
than Original Size.

**`CMP_I16_IN_I32`:** Only the lower 16 bits of each 32-bit input word are
stored. Original Size records the packed 16-bit size, which is half the source
buffer size. The checksum also covers only the packed 16-bit values.

**Checksum:** The checksum is XXH32 with seed 419764627. It is calculated over
16-bit samples in big-endian order or over packed RAW12 bytes. For an odd number
of RAW12 samples, the unused high nibble of the final byte is treated as zero.
