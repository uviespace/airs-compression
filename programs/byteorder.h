/**
 * @file
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date   2015
 * @copyright GPL-2.0
 *
 * @brief This is a set of macros for consistent endianness conversion. They work
 *	for both little and big endian cpus.
 *
 * conversion of XX-bit integers (16- or 32-) between native CPU format
 * and little/big endian format:
 *	cpu_to_[bl]eXX(uintXX_t x)
 *	[bl]eXX_to_cpu(uintXX_t x)
 *
 * the same, but change in situ:
 *	cpu_to_[bl]eXXs(uintXX_t x)
 *	[bl]eXX_to_cpus(uintXX_t x)
 *
 *
 * This is based on the byte order macros from the linux kernel, see:
 * include/linux/byteorder/generic.h
 * include/uapi/linux/swab.h
 * include/uapi/linux/byteorder/big_endian.h
 * include/uapi/linux/byteorder/little_endian.h
 * by @author Linus Torvalds et al.
 *
 */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <stdint.h>

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000		\
		     + __GNUC_MINOR__ * 100	\
		     + __GNUC_PATCHLEVEL__)
#endif

#ifdef __BIG_ENDIAN
#undef __BIG_ENDIAN
#endif

#ifdef __LITTLE_ENDIAN
#undef __LITTLE_ENDIAN
#endif

#ifndef __BIG_ENDIAN
#  if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define __BIG_ENDIAN 4321
#  elif defined(__clang__) && defined(__BIG_ENDIAN__)
#    define __BIG_ENDIAN 4321
#  elif defined(__sparc__)
#    define __BIG_ENDIAN 4321
#  endif
#endif

#ifndef __LITTLE_ENDIAN
#  if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define __LITTLE_ENDIAN 1234
#  elif defined(__clang__) & defined(__LITTLE_ENDIAN__)
#    define __LITTLE_ENDIAN 1234
#  elif defined(_MSC_VER) && (_M_AMD64 || _M_IX86)
#    define __LITTLE_ENDIAN 1234
#  elif defined(__i386__) || defined(__x86_64__)
#    define __LITTLE_ENDIAN 1234
#  endif
#endif

#if defined(__BIG_ENDIAN) == defined(__LITTLE_ENDIAN)
#error "Unknown byte order!"
#endif


#define ___constant_swab16(x) ((uint16_t)(			\
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |		\
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

#define ___constant_swab32(x) ((uint32_t)(			\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |	\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |	\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |	\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define ___constant_swab64(x) ((uint64_t)(				\
	(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |	\
	(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |	\
	(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |	\
	(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |	\
	(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |	\
	(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

#if GCC_VERSION >= 40400 || defined(__clang__)
#define __HAVE_BUILTIN_BSWAP32__
#define __HAVE_BUILTIN_BSWAP64__
#endif
#if GCC_VERSION >= 40800 || defined(__clang__)
#define __HAVE_BUILTIN_BSWAP16__
#endif /* USE_BUILTIN_BSWAP */


static __inline __attribute__((const)) uint16_t __fswab16(uint16_t val)
{
#ifdef __HAVE_BUILTIN_BSWAP16__
	return __builtin_bswap16(val);
#else
	return ___constant_swab16(val);
#endif
}


static __inline __attribute__((const)) uint32_t __fswab32(uint32_t val)
{
#ifdef __HAVE_BUILTIN_BSWAP32__
	return __builtin_bswap32(val);
#else
	return ___constant_swab32(val);
#endif
}


static __inline __attribute__((const)) uint64_t __fswab64(uint64_t val)
{
#ifdef __HAVE_BUILTIN_BSWAP64__
	return __builtin_bswap64(val);
#else
	return ___constant_swab64(val);
#endif
}

/**
 * @brief return a byteswapped 16-bit value
 * @param x value to byteswap
 */

#define __swab16(x)				\
	(__builtin_constant_p((uint16_t)(x)) ?	\
	___constant_swab16(x) :			\
	__fswab16(x))


/**
 * @brief return a byteswapped 32-bit value
 * @param x a value to byteswap
 */

#define __swab32(x)				\
	(__builtin_constant_p((uint32_t)(x)) ?	\
	___constant_swab32(x) :			\
	__fswab32(x))


/**
 * @brief return a byteswapped 64-bit value
 * @param x a value to byteswap
 */

#define __swab64(x)				\
	(__builtin_constant_p((uint64_t)(x)) ?	\
	___constant_swab64(x) :			\
	__fswab64(x))

/**
 * @brief return a byteswapped 16-bit value from a pointer
 * @param p a pointer to a naturally-aligned 16-bit value
 */
static __inline uint16_t __swab16p(const uint16_t *p)
{
	return __swab16(*p);
}


/**
 * @brief return a byteswapped 32-bit value from a pointer
 * @param p a pointer to a naturally-aligned 32-bit value
 */
static __inline uint32_t __swab32p(const uint32_t *p)
{
	return __swab32(*p);
}


/**
 * @brief return a byteswapped 64-bit value from a pointer
 * @param p a pointer to a naturally-aligned 64-bit value
 */
static __inline uint64_t __swab64p(const uint64_t *p)
{
	return __swab64(*p);
}


/**
 * @brief byteswap a 16-bit value in-place
 * @param p a pointer to a naturally-aligned 16-bit value
 */

static __inline void __swab16s(uint16_t *p)
{
	*p = __swab16p(p);
}


/**
 * @brief byteswap a 32-bit value in-place
 * @param p a pointer to a naturally-aligned 32-bit value
 */

static __inline void __swab32s(uint32_t *p)
{
	*p = __swab32p(p);
}


/**
 * @brief byteswap a 64-bit value in-place
 * @param p a pointer to a naturally-aligned 64-bit value
 */

static __inline void __swab64s(uint64_t *p)
{
	*p = __swab64p(p);
}


#ifdef __BIG_ENDIAN

#define __cpu_to_le16(x)   ((uint16_t)__swab16((x)))
#define __cpu_to_le32(x)   ((uint32_t)__swab32((x)))
#define __cpu_to_le64(x)   ((uint64_t)__swab64((x)))

#define __cpu_to_le16s(x)  __swab16s((x))
#define __cpu_to_le32s(x)  __swab32s((x))
#define __cpu_to_le64s(x)  __swab64s((x))

#define __cpu_to_be16(x)   ((uint16_t)(x))
#define __cpu_to_be32(x)   ((uint32_t)(x))
#define __cpu_to_be64(x)   ((uint64_t)(x))

#define __cpu_to_be16s(x)  { (void)(x); }
#define __cpu_to_be32s(x)  { (void)(x); }
#define __cpu_to_be64s(x)  { (void)(x); }



#define __le16_to_cpu(x)   __swab16((uint16_t)(x))
#define __le32_to_cpu(x)   __swab32((uint32_t)(x))
#define __le64_to_cpu(x)   __swab64((uint64_t)(x))

#define __le16_to_cpus(x)  __swab16s((x))
#define __le32_to_cpus(x)  __swab32s((x))
#define __le64_to_cpus(x)  __swab64s((x))

#define __be16_to_cpu(x)   ((uint16_t)(x))
#define __be32_to_cpu(x)   ((uint32_t)(x))
#define __be64_to_cpu(x)   ((uint64_t)(x))

#define __be16_to_cpus(x)  { (void)(x); }
#define __be32_to_cpus(x)  { (void)(x); }
#define __be64_to_cpus(x)  { (void)(x); }

#endif /* __BIG_ENDIAN */


#ifdef __LITTLE_ENDIAN

#define __cpu_to_le16(x)   ((uint16_t)(x))
#define __cpu_to_le32(x)   ((uint32_t)(x))
#define __cpu_to_le64(x)   ((uint64_t)(x))

#define __cpu_to_le16s(x)  { (void)(x); }
#define __cpu_to_le32s(x)  { (void)(x); }
#define __cpu_to_le64s(x)  { (void)(x); }

#define __cpu_to_be16(x)   ((uint16_t)__swab16((x)))
#define __cpu_to_be32(x)   ((uint32_t)__swab32((x)))
#define __cpu_to_be64(x)   ((uint64_t)__swab64((x)))

#define __cpu_to_be16s(x)  __swab16s((x))
#define __cpu_to_be32s(x)  __swab32s((x))
#define __cpu_to_be64s(x)  __swab64s((x))



#define __le16_to_cpu(x)  ((uint16_t)(x))
#define __le32_to_cpu(x)  ((uint32_t)(x))
#define __le64_to_cpu(x)  ((uint64_t)(x))

#define __le64_to_cpus(x) { (void)(x); }
#define __le32_to_cpus(x) { (void)(x); }
#define __le16_to_cpus(x) { (void)(x); }

#define __be16_to_cpu(x)  __swab16((uint16_t)(uint16_t)(x))
#define __be32_to_cpu(x)  __swab32((uint32_t)(uint32_t)(x))
#define __be64_to_cpu(x)  __swab64((uint64_t)(uint64_t)(x))

#define __be16_to_cpus(x) __swab16s((x))
#define __be32_to_cpus(x) __swab32s((x))
#define __be64_to_cpus(x) __swab64s((x))

#endif /* __LITTLE_ENDIAN */



/** these are the conversion macros */

/** convert cpu order to little endian */
#define cpu_to_le16  __cpu_to_le16
#define cpu_to_le32  __cpu_to_le32
#define cpu_to_le64  __cpu_to_le64

/** in-place convert cpu order to little endian */
#define cpu_to_le16s __cpu_to_le16s
#define cpu_to_le32s __cpu_to_le32s
#define cpu_to_le64s __cpu_to_le64s

/** convert cpu order to big endian */
#define cpu_to_be16  __cpu_to_be16
#define cpu_to_be32  __cpu_to_be32
#define cpu_to_be64  __cpu_to_be64

/** in-place convert cpu order to big endian */
#define cpu_to_be16s __cpu_to_be16s
#define cpu_to_be32s __cpu_to_be32s
#define cpu_to_be64s __cpu_to_be64s


/* same, but in reverse */

/** convert little endian to cpu order*/
#define le16_to_cpu  __le16_to_cpu
#define le32_to_cpu  __le32_to_cpu
#define le64_to_cpu  __le64_to_cpu

/** in-place convert little endian to cpu order*/
#define le16_to_cpus __le16_to_cpus
#define le32_to_cpus __le32_to_cpus
#define le64_to_cpus __le64_to_cpus

/** convert big endian to cpu order*/
#define be16_to_cpu  __be16_to_cpu
#define be32_to_cpu  __be32_to_cpu
#define be64_to_cpu  __be64_to_cpu

/** in-place convert big endian to cpu order*/
#define be16_to_cpus __be16_to_cpus
#define be32_to_cpus __be32_to_cpus
#define be64_to_cpus __be64_to_cpus



#endif /* BYTEORDER_H */
