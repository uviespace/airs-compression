/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

/*
 * Modifications made by
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date 2026
 *
 * - Added support for the LEON3 processor, based on @author Armin Luntzer flightos
 * - Modified the code to comply with the Linux kernel coding style.
 *
 * Modifications are also licensed under the same license for consistency
 */


#include <stdint.h>

#include "clock.h"
#include "../lib/common/bithacks.h"

uint64_t clock_get_elapsed_nano(uint64_t start_time)
{
	uint64_t const end_time = clock_get_time();

	return end_time - start_time;
}


#ifdef __sparc__      /* GR712RC LEON3 processor */
#  include <stdlib.h> /* abort */

#  define LEON3_BASE_ADDRESS_GPTIMER 0x80000300
#  define LEON3_BASE_ADDRESS_GRTIMER 0x80100600
#  define CPU_CPS                    80000000 /* in Hz */
#  define GRTIMER_RELOAD             4
#  define GRTIMER_MAX                0xffffffff
#  define GRTIMER_TICKS_PER_SEC      ((CPU_CPS / (GRTIMER_RELOAD + 1)))

struct grtimer_timer {
	uint32_t value;
	uint32_t reload;
	uint32_t ctrl;
	uint32_t latch_value;
} __attribute__((packed));

struct grtimer_unit {
	uint32_t scaler;
	uint32_t scaler_reload;
	uint32_t config;
	uint32_t irq_select;
	struct grtimer_timer timer[2];
} __attribute__((packed));


/**
 * "coarse" contains the counter of the secondary (chained) timer in multiples
 * of seconds and is chained to the "fine" timer, which should hence underflow
 * in a 1-second cycle
 */
struct grtimer_uptime {
	uint32_t coarse;
	uint32_t fine;
};

static int32_t grtimer_longcount_start(struct grtimer_unit *rtu, uint32_t scaler_reload,
				       uint32_t fine_ticks_per_sec, uint32_t coarse_ticks_max);
static void grtimer_longcount_get_uptime(struct grtimer_unit *rtu, struct grtimer_uptime *up);
static double grtimer_longcount_difftime(struct grtimer_unit *rtu, struct grtimer_uptime time1,
					 struct grtimer_uptime time0);

uint64_t clock_get_time(void)
{
	static struct grtimer_unit *rtu = (struct grtimer_unit *)LEON3_BASE_ADDRESS_GRTIMER;
	static struct grtimer_uptime start;
	static int init = 0;

	if (!init) {
		int32_t const err = grtimer_longcount_start(rtu, GRTIMER_RELOAD,
							    GRTIMER_TICKS_PER_SEC, GRTIMER_MAX);
		if (err)
			abort();

		grtimer_longcount_get_uptime(rtu, &start);
		init = 1;
	}
	{
		uint64_t r;
		struct grtimer_uptime up;

		grtimer_longcount_get_uptime(rtu, &up);
		r = (uint64_t)(grtimer_longcount_difftime(rtu, up, start) * 1000000000ULL);
		return r;
	}
}


#  define LEON3_TIMER_EN 0x00000001U /* enable counting */
#  define LEON3_TIMER_RS 0x00000002U /* restart from timer reload value */
#  define LEON3_TIMER_LD 0x00000004U /* load counter    */
#  define LEON3_TIMER_IE 0x00000008U /* irq enable      */
#  define LEON3_TIMER_IP 0x00000010U /* irq pending (clear by writing 0 */
#  define LEON3_TIMER_CH 0x00000020U /* chain with preceding timer */

#  define LEON3_CFG_TIMERS_MASK  0x00000007
#  define LEON3_CFG_IRQNUM_MASK  0x000000f8
#  define LEON3_CFG_IRQNUM_SHIFT 0x3

#  define LEON3_GRTIMER_CFG_LATCH 0x800

static __inline uint32_t ioread32be(const volatile void *addr)
{
	return *(const volatile uint32_t *)addr;
}

static __inline void iowrite32be(uint32_t l, volatile void *addr)
{
#  define ASI_LEON_NOCACHE 0x01 /* force cache miss */
	__asm__ __volatile__("sta	%r0, [%1] %2\n\t"
			     :
			     : "Jr"(l), "r"(addr), "i"(ASI_LEON_NOCACHE)
			     : "memory");
}

/** @brief sets the load flag of a timer */
static void grtimer_set_load(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t flags;

	flags = ioread32be(&rtu->timer[timer].ctrl);
	flags |= LEON3_TIMER_LD;

	iowrite32be(flags, &rtu->timer[timer].ctrl);
}

/** @brief set enable flag in timer */
static void grtimer_set_enabled(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_EN;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}

/** @brief set restart flag in timer */
static void grtimer_set_restart(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_RS;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}

/** @brief set selected timer to chain to the preceding timer */
static void grtimer_set_chained(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_CH;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}

/** @brief set the timer's latch bit */
static void grtimer_enable_latch(struct grtimer_unit *rtu)
{
	uint32_t config;

	config = ioread32be(&rtu->config);
	config |= LEON3_GRTIMER_CFG_LATCH;

	iowrite32be(config, &rtu->config);
}

/**
 * @brief get the time since the long counting grtimer was started
 * @param rtu a struct grtimer_unit
 * @param up a struct grtimer_uptime
 * @note  if configured properly, fine will be in cpu cycles and coarse will
 *        be in seconds
 */
static void grtimer_longcount_get_uptime(struct grtimer_unit *rtu, struct grtimer_uptime *up)
{
	uint32_t t0, t1;
	uint32_t r0, r1;

	uint32_t sc = ioread32be(&rtu->scaler_reload);

	uint32_t t0a = ioread32be(&rtu->timer[0].value);
	uint32_t t1a = ioread32be(&rtu->timer[1].value);

	uint32_t t0b = ioread32be(&rtu->timer[0].value);
	uint32_t t1b = ioread32be(&rtu->timer[1].value);

	uint32_t t0c = ioread32be(&rtu->timer[0].value);
	uint32_t t1c = ioread32be(&rtu->timer[1].value);

	if ((t0a >= t0b) && (t1a >= t1b)) {
		t0 = t0a;
		t1 = t1a;
	} else {
		t0 = t0c;
		t1 = t1c;
	}

	r0 = ioread32be(&rtu->timer[0].reload);
	r1 = ioread32be(&rtu->timer[1].reload);

	up->fine = (r0 - t0) * (sc + 1);
	up->coarse = (r1 - t1);
}

/**
 * @brief enable long count timer
 * @param rtu a struct grtimer_unit
 * @param scaler_reload a scaler reload value
 * @param fine_ticks_per_sec a timer reload value in ticks per second
 * @param coarse_ticks_max a timer reload value in ticks per second
 *
 * If properly configured, grtimer[0] will hold fractions of a second and
 * grtimer[1] will be in seconds, counting down from coarse_ticks_max
 *
 * @return -1 if fine_ticks_per_sec is not an integer multiple of scaler_reload,
 *          0 otherwise
 *
 * @note the return value warns about a configuration error, but will still
 *	 accept the input
 */
static int32_t grtimer_longcount_start(struct grtimer_unit *rtu, uint32_t scaler_reload,
				       uint32_t fine_ticks_per_sec, uint32_t coarse_ticks_max)
{
	iowrite32be(scaler_reload, &rtu->scaler_reload);
	iowrite32be(fine_ticks_per_sec, &rtu->timer[0].reload);
	iowrite32be(coarse_ticks_max, &rtu->timer[1].reload);

	grtimer_set_load(rtu, 0);
	grtimer_set_load(rtu, 1);

	grtimer_set_restart(rtu, 0);
	grtimer_set_restart(rtu, 1);

	grtimer_set_chained(rtu, 1);

	grtimer_set_enabled(rtu, 0);
	grtimer_set_enabled(rtu, 1);

	grtimer_enable_latch(rtu);

	/* not an integer multiple, clock will drift */
	if (!is_multiple_of(fine_ticks_per_sec, scaler_reload))
		return -1;

	return 0;
}

/**
 * @brief get the number of seconds elapsed between timestamps taken from the
 *	longcount timer
 *
 * @param time1 a struct grtime_uptime
 * @param time0 a struct grtime_uptime
 *
 * @return time difference in seconds represented as double
 */
static double grtimer_longcount_difftime(struct grtimer_unit *rtu, struct grtimer_uptime time1,
					 struct grtimer_uptime time0)
{
	uint32_t sc = ioread32be(&rtu->scaler_reload);
	uint32_t rl = ioread32be(&rtu->timer[0].reload);

	double cpu_freq = (double)(sc + 1) * rl;

	double t0 = (double)time0.coarse + (double)time0.fine / cpu_freq;
	double t1 = (double)time1.coarse + (double)time1.fine / cpu_freq;

	return t1 - t0;
}

#elif defined(__APPLE__) && defined(__MACH__)
#  include <mach/mach_time.h> /* mach_timebase_info_data_t, mach_timebase_info, mach_absolute_time */

uint64_t clock_get_time(void)
{
	static mach_timebase_info_data_t rate;
	static int init = 0;

	if (!init) {
		mach_timebase_info(&rate);
		init = 1;
	}

	return mach_absolute_time() * (uint64_t)rate.numer / (uint64_t)rate.denom;
}

#else
#  include <time.h>
/* POSIX.1-2001 (optional) */
#  if defined(CLOCK_MONOTONIC)
#    include <stdio.h>
#    include <stdlib.h>

uint64_t clock_get_time(void)
{
	/* time must be initialized, otherwise MSAN may fail. */
	struct timespec time = { 0, 0 };

	if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
		perror("timefn::clock_gettime(CLOCK_MONOTONIC)");
		abort();
	}
	return (uint64_t)time.tv_sec * 1000000000ULL + (uint64_t)time.tv_nsec;
}
#  else
#    error ("I need a stop watch!")
#  endif
#endif
