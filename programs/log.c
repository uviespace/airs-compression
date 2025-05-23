/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Logging functionality implementation
 */

#include <stdlib.h>

#include "util.h"
#include "log.h"

/**
 * This global variable holds the current configuration for logging
 */
static struct log_state {
	enum log_level current_level;
	enum log_color_status color_status;
} g_log_state = { LOG_LEVEL_DEFAULT, LOG_COLOR_DEFAULT };


void log_setup_color(void)
{
#if defined(_WIN32) || defined(_WIN64)
	/* Windows gets no color! */
	log_state.color_status = LOG_COLOR_DISABLED;
#else
	const char *no_color = getenv("NO_COLOR");
	const char *force_color = getenv("CLICOLOR_FORCE");
	const char *cli_color = getenv("CLICOLOR");

	if (no_color != NULL && no_color[0] != '\0') {
		g_log_state.color_status = LOG_COLOR_DISABLED;
		return;
	}

	if (force_color != NULL && force_color[0] != '\0') {
		g_log_state.color_status = LOG_COLOR_ENABLED;
		return;
	}

	if (cli_color != NULL && cli_color[0] == '0') {
		g_log_state.color_status = LOG_COLOR_DISABLED;
		return;
	}

	if (util_is_console(LOG_STREAM))
		g_log_state.color_status = LOG_COLOR_ENABLED;
	else
		g_log_state.color_status = LOG_COLOR_DISABLED;
#endif
}


void log_increase_verbosity(void)
{
	if (g_log_state.current_level < LOG_LEVEL_MAX)
		g_log_state.current_level++;
}


void log_decrease_verbosity(void)
{
	if (g_log_state.current_level > LOG_LEVEL_QUIET)
		g_log_state.current_level--;
}


void log_set_level(enum log_level level)
{
	g_log_state.current_level = level;
}


enum log_level log_get_level(void)
{
	return g_log_state.current_level;
}


void log_set_color(enum log_color_status status)
{
	g_log_state.color_status = status;
}


enum log_color_status log_get_color(void)
{
	return g_log_state.color_status;
}
