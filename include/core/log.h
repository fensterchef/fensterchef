#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#include "utility/utility.h"

typedef enum {
    /* everything gets logged */
    LOG_SEVERITY_ALL,
    /* only information gets logged */
    LOG_SEVERITY_INFO,
    /* only errors get logged */
    LOG_SEVERITY_ERROR,
    /* log nothing */
    LOG_SEVERITY_NOTHING,
} log_severity_t;

/* the severity of the logging */
extern log_severity_t log_severity;

/* the path of the log file */
extern char *log_file_path;

/* the file to log to */
extern FILE *log_file;

#ifdef DEBUG

/* ansi color codes for colored output */
# define ANSI_BLACK 0
# define ANSI_RED 1
# define ANSI_GREEN 2
# define ANSI_YELLOW 3
# define ANSI_BLUE 4
# define ANSI_MAGENTA 5
# define ANSI_CYAN 6
# define ANSI_WHITE 7

/* clear the current visual attributes */
# define CLEAR_COLOR "\x1b[0m"
/* make the text bold */
# define BOLD_COLOR "\x1b[1m"
/* set the foreground color */
# define COLOR(color) "\x1b[3" STRINGIFY(ANSI_##color) "m"

#else

# define CLEAR_COLOR ""
# define BOLD_COLOR ""
# define COLOR(color) ""

#endif

#ifdef DEBUG
# define LOG_DEBUG LOG
#else
# define LOG_DEBUG(...)
#endif

/* printf format specifiers that can be used */
#define PRINTF_FORMAT_SPECIFIERS "diuoxfegcsp"

#ifdef DEBUG
void _log_formatted(log_severity_t severity, const char *file, int line,
        const char *format, ...);

# define LOG_VERBOSE(...) \
    _log_formatted(LOG_SEVERITY_ALL, __FILE__, __LINE__, __VA_ARGS__)
# define LOG(...) \
    _log_formatted(LOG_SEVERITY_INFO, __FILE__, __LINE__, __VA_ARGS__)
# define LOG_ERROR(...) \
    _log_formatted(LOG_SEVERITY_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#else

void _log_formatted(log_severity_t severity, const char *format, ...);

# define LOG_VERBOSE(...) \
    _log_formatted(LOG_SEVERITY_ALL, __VA_ARGS__)
# define LOG(...) \
    _log_formatted(LOG_SEVERITY_INFO, __VA_ARGS__)
# define LOG_ERROR(...) \
    _log_formatted(LOG_SEVERITY_ERROR, __VA_ARGS__)
#endif

/* Print a formatted string to standard error output.
 *
 * The following format specifiers are supported on top of the regular
 * format specifiers (some printf format specifiers might be overwritten):
 * %P   int, int                X+Y
 * %S   unsigned, unsigned      WIDTHxHEIGHT
 * %R   unsigned[4]             X+Y+WIDTHxHEIGHT
 * %w   Window                  ID<NUMBER or NAME>
 * %W   Window*                 ID<NUMBER or NAME>
 * %m   window_state_t          WINDOW_STATE
 * %F   Frame*                  (X+Y+WIDTHxHEIGHT)
 * %A   struct action_list*     ACTION LIST
 * %T   struct parse_data*      PARSE DATA
 * %a   Atom                    ATOM
 * %V   XEvent*                 EVENT
 * %E   XError*                 ERROR
 * %D   Display*                DISPLAY
 */
void log_formatted(const char *format, ...);

#endif
