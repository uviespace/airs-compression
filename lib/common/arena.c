/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * This must live in a single .c file. A static variable in a header would
 * create one oom-handler per translation unit, not a single program-wide one.
 */

#include <stdlib.h>

#include "arena.h"

ARENA_NORETURN static void arena_default_oom(void)
{
	exit(3);
}


static void (*arena_oom_handler)(void) = &arena_default_oom;


ARENA_NORETURN void arena_oom(void)
{
	arena_oom_handler();
	abort();
}


void arena_set_oom_handler(void (*handler)(void))
{
	if (handler)
		arena_oom_handler = handler;
	else
		arena_oom_handler = &arena_default_oom;
}
