/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Big-Endian Bitstream Writer
 *
 * Usage:
 * - Initialize the bitstream writer:
 *        error_code = bitstream_write_init();
 * - Write bits to the bitstream:
 *        error_code = bitstream_write();
 * - Flush remaining bits to the buffer:
 *        bytes_written = bitstream_flush();
 */

#ifndef CMP_BITSTREAM_WRITER_H
#define CMP_BITSTREAM_WRITER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../common/err_private.h"
#include "../common/byteorder.h"

#define CMP_DST_ALIGNMENT sizeof(uint64_t)


/**
 * @brief This structure maintains the state of the bitstream writer
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the structure.
 */

struct bitstream_writer {
	uint64_t cache;       /**< Local bit cache  */
	unsigned int bit_cap; /**< Bit capacity left in the cache */
	uint8_t *start;       /**< Beginning of bitstream */
	uint8_t *ptr;         /**< Current write position */
	uint8_t *end;         /**< End of the bitstream pointer */
	uint32_t error;       /**< Sticky error code */
};


/**
 * @brief Initializes a bitstream writer
 *
 * @param bs	pointer to an already allocated bitstream_writer structure
 * @param dst	start address of the bitstream buffer; has to be 8-byte aligned
 * @param size	capacity of the bitstream buffer in bytes
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_writer_init(struct bitstream_writer *bs, void *dst,
					       uint32_t size)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	memset(bs, 0, sizeof(*bs));

	if (!dst)
		return bs->error = CMP_ERROR(DST_NULL);
	if ((uintptr_t)dst & (CMP_DST_ALIGNMENT - 1))
		return bs->error = CMP_ERROR(DST_UNALIGNED);

	bs->cache = 0;
	bs->bit_cap = 64;
	bs->start = dst;
	bs->ptr = dst;
	bs->end = (uint8_t *)dst + size;
	return bs->error = CMP_ERROR(NO_ERROR);
}


/**
 * @brief Stores a 64-bit integer as big-endian bytes
 *
 * @param ptr	8 byte aligned address to write
 * @param val	value to write
 */

static __inline void put_be64_aligned(void *ptr, uint64_t val)
{
	val = cpu_to_be64(val);
	*(uint64_t *)ptr = val;
}


/**
 * @brief Get the current error status of the bitstream writer
 *
 * @returns the first occurred error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_error(const struct bitstream_writer *bs)
{
	if (!bs)
		return CMP_ERROR(INT_BITSTREAM);
	return bs->error;
}


/**
 * @brief Adds up to 32 bits to the bitstream
 *
 * @note This function writes bits into an internal cache, which is only flushed to
 *       the output buffer when full or when bitstream_flush() is explicitly called.
 *       As a result, after completing a sequence of writes, the caller **must** call
 *       bitstream_flush() to ensure all bits are properly written to the buffer.
 * @note This function uses sticky error handling - once an error occurs, subsequent
 *       calls are ignored. Possible error conditions can be tested with
 *       bitstream_error() or bitstream_flush().
 *
 * @param bs		pointer to an initialised bitstream_writer structure
 * @param value		bits to write to the bitstream; must be "clean", meaning
 *			all high bits above nbBits are 0
 * @param nb_bits	number of bits to write from value; must be <= 32
 */

static __inline void bitstream_add_bits32(struct bitstream_writer *bs, uint32_t value,
					  unsigned int nb_bits)
{
	if (cmp_is_error_int(bitstream_error(bs)))
		return;

	if (nb_bits > 32) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}
	if (nb_bits < 32 && (value >> nb_bits)) {
		bs->error = CMP_ERROR(INT_BITSTREAM);
		return;
	}

	/* Fast path: bits fit in current cache */
	if (nb_bits < bs->bit_cap) {
		bs->cache = (bs->cache << nb_bits) | value;
		bs->bit_cap -= nb_bits;
		return;
	}

	/* Slow path: need to flush cache */
	if (bs->end - bs->ptr >= 8) {
		bs->cache <<= bs->bit_cap;
		bs->cache |= value >> (nb_bits - bs->bit_cap);
		put_be64_aligned(bs->ptr, bs->cache);

		bs->ptr += 8;
		bs->cache = value;
		bs->bit_cap += 64 - nb_bits;
	} else {
		bs->error = CMP_ERROR(DST_TOO_SMALL);
	}
}


/**
 * @brief Flushes remaining bits form the internal cache to the buffer
 * Last byte may be padded with zeros
 *
 * @param bs	pointer to a initialised bitstream_writer structure
 *
 * @returns written bytes to bitstream or an error code, which can be checked
 *	using cmp_is_error()
 */

static __inline uint32_t bitstream_flush(struct bitstream_writer *bs)
{
	unsigned int bytes;
	uint8_t *cursor;

	if (cmp_is_error_int(bitstream_error(bs)))
		return bitstream_error(bs);

	cursor = bs->ptr;
	bytes = (64 - bs->bit_cap + 7) / 8;
	if (bytes) {
		uint64_t tmp = bs->cache << bs->bit_cap;

		while (bytes--) {
			if (cursor >= bs->end)
				return bs->error = CMP_ERROR(DST_TOO_SMALL);
			*cursor++ = (uint8_t)(tmp >> (64 - 8));
			tmp <<= 8;
		}
	}

	return (uint32_t)(cursor - bs->start);
}


/**
 * @brief Calculates current total written size in bytes
 *
 * @param bs	pointer to the initialised bitstream_writer structure
 *
 * @returns total bytes effectively written including non-flushed cached bits or
 *	an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_size(const struct bitstream_writer *bs)
{
	if (cmp_is_error_int(bitstream_error(bs)))
		return bitstream_error(bs);

	return (uint32_t)(bs->ptr - bs->start) + (64 - (uint32_t)bs->bit_cap + 7) / 8;
}


/**
 * @brief Reset the bitstream writer to the beginning of its buffer
 *
 * @param bs	pointer to the initialised bitstream_writer structure
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

static __inline uint32_t bitstream_rewind(struct bitstream_writer *bs)
{
	uint32_t const ret = bitstream_flush(bs);

	if (cmp_is_error_int(ret))
		return ret;

	return bitstream_writer_init(bs, bs->start, (uint32_t)(bs->end - bs->start));
}


#endif /* CMP_BITSTREAM_WRITER_H */
