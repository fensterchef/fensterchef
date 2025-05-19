#include <stdlib.h>

#include "configuration.h"
#include "event.h"
#include "fensterchef.h"
#include "log.h"
#include "monitor.h"
#include "x11/display.h"
#include "x11/synchronize.h"

/* all possible program options */
typedef enum program_option {
    PROGRAM_OPTION_HELP, /* -h, --help */
    PROGRAM_OPTION_USAGE, /* --usage */
    PROGRAM_OPTION_VERSION, /* -v, --version */
    PROGRAM_OPTION_VERBOSITY, /* -d, --verbosity VERBOSITY */
    PROGRAM_OPTION_VERBOSE, /* --verbose */
    PROGRAM_OPTION_CONFIG, /* --config FILE */
    PROGRAM_OPTION_COMMAND, /* -e, --command COMMAND... */
} program_option_t;

/* context the parser needs to parse the options */
static const struct program_parse_option {
    /* a short form of the option after `-` */
    char short_option;
    /* the long option after `--`  */
    const char *long_option;
    /* what kind of arguments to expect */
    /* 0 - no arguments */
    /* 1 - single argument */
    int type;
} parse_options[] = {
    [PROGRAM_OPTION_HELP] = { 'h', "help", 0 },
    [PROGRAM_OPTION_USAGE] = { '\0', "usage", 0 },
    [PROGRAM_OPTION_VERSION] = { 'v', "version", 0 },
    [PROGRAM_OPTION_VERBOSITY] = { 'd', "verbosity", 1 },
    [PROGRAM_OPTION_VERBOSE] = { '\0', "verbose", 0 },
    [PROGRAM_OPTION_CONFIG] = { '\0', "config", 1 },
    [PROGRAM_OPTION_COMMAND] = { 'e', "command", 1 },
};

/* the program arguments and their count */
static char **program_arguments;
static int number_of_program_arguments;

/* the current index of the argument being parsed */
static int program_argument_index;

/* Print the usage to standard error output and exit. */
static void print_usage(int exit_code)
{
    fprintf(stderr, "Usage: %s [OPTIONS...]\n",
            program_arguments[0]);
    fputs("Options:\n\
        -h, --help                  show this help\n\
        -v, --version               show the version\n\
        -d, --verbosity VERBOSITY   set the logging verbosity\n\
            all                     log everything\n\
            info                    only log informational messages\n\
            error                   only log errors\n\
            nothing                 log nothing\n\
        --verbose                   log everything\n\
        --config        FILE        set the path of the configuration\n\
        -e, --command   COMMAND     run a command within fensterchef\n",
        stderr);
    exit(exit_code);

}

/* Print the version to standard error output and exit. */
static void print_version(void)
{
    fputs("fensterchef " FENSTERCHEF_VERSION "\n", stderr);
    exit(EXIT_SUCCESS);
}

/* Handles given option with given argument @value. */
static void handle_program_option(program_option_t option, char *value)
{
    const char *verbosities[] = {
        [LOG_SEVERITY_ALL] = "all",
        [LOG_SEVERITY_INFO] = "info",
        [LOG_SEVERITY_ERROR] = "error",
        [LOG_SEVERITY_NOTHING] = "nothing",
    };

    LOG_DEBUG("option %s (%c): %s\n",
            parse_options[option].long_option,
            parse_options[option].short_option, value);

    switch (option) {
    /* the user wants to receive information */
    case PROGRAM_OPTION_HELP:
    case PROGRAM_OPTION_USAGE:
        print_usage(EXIT_SUCCESS);
        break;

    /* print the version */
    case PROGRAM_OPTION_VERSION:
        print_version();
        break;

    /* change of logging verbosity */
    case PROGRAM_OPTION_VERBOSITY:
        for (log_severity_t i = 0; i < SIZE(verbosities); i++) {
            if (strcasecmp(verbosities[i], value) == 0) {
                log_severity = i;
                return;
            }
        }

        fprintf(stderr, "invalid verbosity, pick one of: %s", verbosities[0]);
        for (log_severity_t i = 1; i < SIZE(verbosities); i++) {
            fprintf(stderr, ", %s", verbosities[i]);
        }
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
        break;

    /* verbose logging */
    case PROGRAM_OPTION_VERBOSE:
        log_severity = LOG_SEVERITY_ALL;
        break;

    /* set the configuration */
    case PROGRAM_OPTION_CONFIG:
        free(Fensterchef_configuration);
        Fensterchef_configuration = xstrdup(value);
        break;

    /* run a command */
    case PROGRAM_OPTION_COMMAND: {
        size_t length;
        char *command;

        length = strlen(value);
        command = xmemdup(value, length + 1);
        /* combine all arguments space separated */
        for (int i = program_argument_index;
                i < number_of_program_arguments;
                i++) {
            const size_t n = strlen(program_arguments[i]);
            REALLOCATE(command, length + 1 + n + 1);
            command[length] = ' ';
            length++;
            memcpy(&command[length], program_arguments[i], n);
            length += n;
            command[length] = '\0';
        }
        run_external_command(command);
        break;
    }

    default:
        print_usage(EXIT_FAILURE);
        break;
    }
}

/* Parse the set program arguments. */
void parse_program_arguments(void)
{
    char *argument, *value;
    bool is_long;

    argument = program_arguments[1];
    for (int i = 1; i < number_of_program_arguments; ) {
        program_option_t option = 0;

        if (argument[0] != '-') {
            fprintf(stderr, "argument \"%s\" is unexpected\n",
                    argument);
            print_usage(EXIT_FAILURE);
        }

        /* check for a long option */
        if (argument[1] == '-') {
            char *equality;

            /* jump over the leading `--` */
            argument += 2;

            /* check for a following `=` and make it a null terminator */
            equality = strchr(argument, '=');
            if (equality != NULL) {
                equality[0] = '\0';
            }

            /* try to find the long option */
            for (; option < SIZE(parse_options); option++) {
                if (strcmp(parse_options[option].long_option,
                            &argument[0]) == 0) {
                    /* either go the end of the argument... */
                    if (equality == NULL) {
                        argument += strlen(argument);
                    /* ...or jump after the equals sign */
                    } else {
                        argument = equality + 1;
                    }
                    break;
                }
            }

            is_long = true;
        } else {
            /* jump over the leading `-` */
            argument++;

            /* try to find the short option */
            for (; option < SIZE(parse_options); option++) {
                if (parse_options[option].short_option == argument[0]) {
                    argument++;
                    break;
                }
            }

            is_long = false;
        }

        /* check if the option exists */
        if (option >= SIZE(parse_options)) {
            fprintf(stderr, "invalid option %s\n", argument);
            print_usage(EXIT_FAILURE);
        }

        value = NULL;

        /* if the option expects no arguments */
        if (parse_options[option].type == 0) {
            /* if the option had the form `--option=value` */
            if (argument[0] != '\0' && is_long) {
                fprintf(stderr, "did not expect argument %s\n", argument);
                print_usage(EXIT_FAILURE);
            }
        } else {
            /* try to take the next argument if the current one has no more */
            if (argument[0] == '\0') {
                i++;
                if (i == number_of_program_arguments) {
                    fprintf(stderr, "expected argument\n");
                    print_usage(EXIT_FAILURE);
                }
                value = program_arguments[i];
            } else {
                value = argument;
                argument += strlen(argument);
            }
        }

        /* go to the next argument */
        if (argument[0] != '\0') {
            argument--;
            argument[0] = '-';
        } else {
            i++;
            if (i < number_of_program_arguments) {
                argument = program_arguments[i];
            }
        }

        program_argument_index = i;
        handle_program_option(option, value);
    }
}

/* FENSTERCHEF main entry point. */
int main(int argc, char **argv)
{
    Fensterchef_home = getenv("HOME");
    if (Fensterchef_home == NULL) {
        Fensterchef_home = ".";
        LOG_ERROR("HOME is not set, using the arbritary %s\n",
                Fensterchef_home);
    }

    /* parse the program arguments */
    program_arguments = argv;
    number_of_program_arguments = argc;
    parse_program_arguments();

    LOG("parsed arguments, starting to log\n");
    LOG("welcome to " COLOR(YELLOW) FENSTERCHEF_NAME " " COLOR(GREEN)
                FENSTERCHEF_VERSION CLEAR_COLOR "\n");
    LOG("the configuration file resides in %s\n",
            get_configuration_file());

    /* set the signal handlers (very crucial because if we do not handle
     * `SIGALRM`, fensterchef quits)
     */
    initialize_signal_handlers();

    /* open connection to the X server */
    open_connection();

    /* try to take control of the window manager role */
    take_control();

    /* initialize randr if possible and the initial monitors with their
     * root frames
     */
    initialize_monitors();

    /* set the X properties on the root window */
    initialize_root_properties();

    /* configure the monitor frames before running the startup actions */
    reconfigure_monitor_frames();

    /* load the user configuration or the default configuration */
    reload_configuration();

    if (!Fensterchef_is_running) {
        LOG("startup interrupted by user configuration\n");
        quit_fensterchef(EXIT_FAILURE);
    }

    /* manage the windows that are already there */
    query_existing_windows();

    /* do an inital synchronization */
    synchronize_with_server();

    /* run the main event loop */
    run_event_loop();
}
