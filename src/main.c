#include <stdlib.h>

#include "configuration.h"
#include "event.h"
#include "fensterchef.h"
#include "log.h"
#include "monitor.h"
#include "program_options.h"
#include "x11/display.h"
#include "x11/synchronize.h"

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
    parse_program_arguments(argc, argv);

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

    /* manage the windows that are already there */
    query_existing_windows();

    /* configure the monitor frames before running the startup actions */
    reconfigure_monitor_frames();

    /* load the user configuration or the default configuration */
    reload_configuration();

    if (!Fensterchef_is_running) {
        LOG("startup interrupted by user configuration\n");
        quit_fensterchef(EXIT_FAILURE);
    }

    /* do an inital synchronization */
    synchronize_with_server();

    /* before entering the loop, flush all the initialization calls */
    XFlush(display);

    /* run the main event loop */
    run_event_loop();
}
