# Compression Design {#design}

AIRSPACE is a configurable, lossless compression library for science data. Each
compression pass has two steps:

1. **Preprocessing** transforms the input to reduce its entropy.
2. **Encoding** converts the preprocessed values into the compressed bitstream.

In this documentation, a **compression pass** is one compression operation and
its output. A **compression sequence** is one primary compression pass and its
secondary compression passes.

The library uses one **primary compression pass** followed by optional
**secondary compression passes**. Primary and secondary passes can use
different preprocessing and encoder settings. Each pass produces compressed
data consisting of a [compression header](@ref format) and payload.

## Preprocessing Configuration {#design_preprocessing}

Preprocessing is selected with
[`primary_preprocessing`](@ref cmp_params::primary_preprocessing) and
[`secondary_preprocessing`](@ref cmp_params::secondary_preprocessing).

### [`CMP_PREPROCESS_NONE (0)`](@ref CMP_PREPROCESS_NONE)

No transformation is applied.

### [`CMP_PREPROCESS_DIFF (1)`](@ref CMP_PREPROCESS_DIFF)

Computes first-order differences between consecutive samples:

```text
output[0] = input[0]
output[i] = input[i] - input[i - 1]
```

The input is treated as a one-dimensional array. Arithmetic wraps at the source
bit depth. This method needs no working buffer.

### [`CMP_PREPROCESS_IWT (2)`](@ref CMP_PREPROCESS_IWT)

Applies a reversible, multi-level integer wavelet transform. It separates
detail and approximation coefficients and is suitable for data with correlated
structures at several scales. The implementation follows Equation (5.24) in
David Salomon, *Data Compression: The Complete Reference*, 4th ed., Springer,
2007, pp. 608-610 ([DOI: 10.1007/978-1-84628-603-2](https://doi.org/10.1007/978-1-84628-603-2)).

### [`CMP_PREPROCESS_MODEL (3)`](@ref CMP_PREPROCESS_MODEL) {#design_preprocessing_model}

Reduces data entropy by subtracting predicted samples from input samples. The
predictions come from a model built during previous compression passes. This
method is only valid as secondary preprocessing because it requires data from
the primary compression pass.

For each sample at position `i` in secondary compression pass `t`, the
preprocessor computes:

```text
output[i,t] = input[i,t] - model[i,t]
```

Here, `output` is the output residual, `input` is the current input sample, and
`model` is the model prediction.

After each prediction, the model is updated for the next secondary compression
pass using a weighted average:

```text
model[i,t+1] = floor((model[i,t] * model_rate
                      + input[i,t] * (16 - model_rate)) / 16)
```

[`model_rate`](@ref cmp_params::model_rate) is in the range [0, 16]. Low values
adapt quickly, high values retain more of the preceding model. A value of 16
keeps the initial model unchanged.

The primary compression pass provides the initial model:

```text
model[i,0] = input[i,primary]
```

The sample count and signedness must remain constant within a compression
sequence. `CMP_I16` and `CMP_I16_IN_I32` may be mixed, as may `CMP_U16` and
`CMP_RAW12`.

## Encoder Configuration {#design_encoder}

The encoder performs entropy coding and generates the compressed bitstream. It
is selected with
[`primary_encoder_type`](@ref cmp_params::primary_encoder_type) and
[`secondary_encoder_type`](@ref cmp_params::secondary_encoder_type).

|   Value | Encoder                    |                                 Configuration |
| ------: | -------------------------- | --------------------------------------------- |
|       0 | `CMP_ENCODER_UNCOMPRESSED` | Pass-through mode: the data is not compressed |
|       1 | `CMP_ENCODER_GOLOMB_ZERO`  |   Golomb encoder with a zero escape mechanism |
|       2 | `CMP_ENCODER_GOLOMB_MULTI` |  Golomb encoder with a multi-escape mechanism |

The Golomb parameter is configured with
[`primary_encoder_param`](@ref cmp_params::primary_encoder_param) and
[`secondary_encoder_param`](@ref cmp_params::secondary_encoder_param). The
outlier threshold for `CMP_ENCODER_GOLOMB_MULTI` is configured with
[`primary_encoder_outlier`](@ref cmp_params::primary_encoder_outlier) and
[`secondary_encoder_outlier`](@ref cmp_params::secondary_encoder_outlier).
These parameters are ignored by `CMP_ENCODER_UNCOMPRESSED`.

### Golomb Parameter

The valid range is [1, 65535]. Smaller values are suitable for data clustered
close to zero, larger values are suitable for a broader distribution.

### Outlier Threshold

The outlier threshold is applied after signed values are mapped to unsigned
values with ZigZag encoding. Mapped values below the threshold are
Golomb-encoded, mapped values at or above it use an escape representation.
`CMP_ENCODER_GOLOMB_ZERO` calculates this threshold automatically and ignores
the configured value.

`CMP_ENCODER_GOLOMB_MULTI` requires a configured outlier threshold in the range
[1, 4294967295]. A lower threshold uses escapes more often, a higher threshold
uses Golomb codewords more often.

The configured threshold may be reduced to satisfy encoder limits. The header
stores the effective outlier threshold used for the payload.

## Multi-Pass Configuration {#design_multi_pass}

[`secondary_iterations`](@ref cmp_params::secondary_iterations) sets the maximum
number of secondary compression passes. Its range is [0, 255]. Zero disables
secondary passes.

```text
compression sequence A (identifier A):
  primary pass (sequence number 0)
  -> secondary pass (sequence number 1)
  -> ...
  -> secondary pass (sequence number N)

compression sequence B (identifier B):
  primary pass (sequence number 0)
```

All compression passes in a sequence use the same identifier. The sequence
number starts at zero and increments for each secondary pass. After the
configured number of secondary passes, the next compression pass starts a new
sequence and uses the primary configuration. `cmp_reset()` also starts a new
sequence.

Only [MODEL preprocessing](@ref design_preprocessing_model) depends on preceding
compression-pass data. Other secondary configurations follow the same schedule
but do not use previous payloads.

## Additional Options

**[`checksum_enabled`](@ref cmp_params::checksum_enabled)** stores an checksum
of the original packed data in the header. The header field is zero when no
checksums were used.

**[`uncompressed_fallback_enabled`](@ref cmp_params::uncompressed_fallback_enabled)**
prevents data expansion. If a compressed pass would exceed the header plus
original packed size, the context is reset and the data is stored as a new
uncompressed primary pass. The reset discards any model state changed by the
failed attempt.

## Memory Model {#design_memory_model}

The compression library performs no dynamic allocation. The caller owns the
context, source, destination, and working buffers. Use `cmp_cal_work_buf_size()`
for the working-buffer size and `cmp_compress_bound()` for the general
destination bound. A context and its working buffer must not be used by
concurrent compression passes.
