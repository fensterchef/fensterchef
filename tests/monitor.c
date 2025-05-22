#include <stdio.h>
#include <string.h>

#include "frame.h"
#include "monitor.h"
#include "test.h"

struct {
    const char *name;
    Rectangle rectangle;
} monitors[] = {
    { "Main", { 0, 0, 800, 600 } },
    { "Left", { -810, 0, 800, 600 } },
    { "Right", { 810, 0, 800, 600 } },
    { "Top", { 0, -610, 800, 600 } },
    { "Bottom", { 0, 610, 800, 600 } },
    { "FarLeft", { -5000, 0, 800, 600 } },
    { "FarRight", { 5000, 0, 800, 600 } },
    { "FarTop", { 0, -5000, 800, 600 } },
    { "FarBottom", { 0, 5000, 800, 600 } },
    { "Disconnected1", { 3000, 3000, 800, 600 } },
    { "Disconnected2", { -3000, -3000, 800, 600 } },
    { "Small1", { 200, 200, 400, 300 } },
    { "Small2", { -500, 100, 400, 300 } },
    { "Tall", { 1000, 200, 400, 1000 } },
    { "Wide", { 100, 1000, 1200, 400 } },
    { "Diagonal1", { -700, -700, 800, 600 } },
    { "Diagonal2", { 700, 700, 800, 600 } },
};

int monitor_directions(void)
{
    struct left_relations {
        const char *left;
        const char *right;
    } left_relations[] = {
        { "Small1", "Main" },
        { "Small2", "Left" },
        { "Tall", "Right" },
        { "Diagonal1", "Top" },
        { "Left", "Bottom" },
        { NULL, "FarLeft" },
        { "Right", "FarRight" },
        { "Disconnected2", "FarTop" },
        { "Left", "FarBottom" },
        { "FarBottom", "Disconnected1" },
        { "FarLeft", "Disconnected2" },
        { "Left", "Small1" },
        { "FarLeft", "Small2" },
        { "Wide", "Tall" },
        { "Bottom", "Wide" },
        { "Left", "Diagonal1" },
        { "Tall", "Diagonal2" },
    };

    struct above_relations {
        const char *above;
        const char *below;
    } above_relations[] = {
        { "Small1", "Main" },
        { "Small2", "Left" },
        { "Top", "Right" },
        { "Diagonal1", "Top" },
        { "Main", "Bottom" },
        { "Disconnected2", "FarLeft" },
        { "Top", "FarRight" },
        { NULL, "FarTop" },
        { "Wide", "FarBottom" },
        { "FarRight", "Disconnected1" },
        { "FarTop", "Disconnected2" },
        { "Top", "Small1" },
        { "Diagonal1", "Small2" },
        { "Right", "Tall" },
        { "Diagonal2", "Wide" },
        { "FarTop", "Diagonal1" },
        { "Bottom", "Diagonal2" },
    };

    for (unsigned i = 0; i < SIZE(left_relations); i++) {
        Monitor *left = NULL, *right;

        right = get_monitor_by_name(left_relations[i].right);
        if (left_relations[i].left != NULL) {
            left = get_monitor_by_name(left_relations[i].left);
        }

        if (get_left_monitor(right) != left) {
            if (left == NULL) {
                LOG_ERROR("expected nothing to be left of %s\n",
                        right->name);
            } else {
                LOG_ERROR("expected %s to be left of %s\n",
                        left_relations[i].left, right->name);
            }
            return 1;
        }
    }

    for (unsigned i = 0; i < SIZE(above_relations); i++) {
        Monitor *above = NULL, *below;

        below = get_monitor_by_name(above_relations[i].below);
        if (above_relations[i].above != NULL) {
            above = get_monitor_by_name(above_relations[i].above);
        }

        if (get_above_monitor(below) != above) {
            if (above == NULL) {
                LOG_ERROR("expected nothing to be above of %s\n",
                        below->name);
            } else {
                LOG_ERROR("expected %s to be above of %s\n",
                        above_relations[i].above, below->name);
            }
            return 1;
        }
    }

    return 0;
}

int containing_frame(void)
{
    for (Monitor *monitor = Monitor_first;
            monitor != NULL;
            monitor = monitor->next) {
        if (get_monitor_containing_frame(monitor->frame) != monitor) {
            LOG_ERROR("monitor root frame not within the monitor?\n");
            return 1;
        }
    }
    return 0;
}

int monitor_pattern(void)
{
    const struct {
        const char *pattern;
        Monitor *monitor;
    } test_cases[] = {
        { "M*", get_monitor_by_name("Main") },
        { "?ef?", get_monitor_by_name("Left") },
        { "Small[1-2]", get_monitor_by_name("Small1") },
        { "Far*", get_monitor_by_name("FarLeft") },
        { "*Bottom", get_monitor_by_name("Bottom") },
        { "Diagonal?", get_monitor_by_name("Diagonal1") },
    };

    for (unsigned i = 0; i < SIZE(test_cases); i++) {
        Monitor *const monitor = get_monitor_by_pattern(test_cases[i].pattern);
        if (monitor != test_cases[i].monitor) {
            LOG_ERROR("pattern %s has wrong monitor %s\n",
                    test_cases[i].pattern,
                    monitor == NULL ? "(null)" : monitor->name);
            return 1;
        }
    }
    return 0;
}

int monitor_from_rectangle(void)
{
    const struct {
        Rectangle rectangle;
        Monitor *monitor;
    } test_cases[] = {
        { { 220, 300, 350, 80 }, get_monitor_by_name("Main") },
    };

    for (unsigned i = 0; i < SIZE(test_cases); i++) {
        Monitor *const monitor = get_monitor_from_rectangle(
                test_cases[i].rectangle.x,
                test_cases[i].rectangle.y,
                test_cases[i].rectangle.width,
                test_cases[i].rectangle.height);
        if (monitor != test_cases[i].monitor) {
            LOG_ERROR("rectangle %R has wrong monitor %s\n",
                    test_cases[i].rectangle.x,
                    test_cases[i].rectangle.y,
                    test_cases[i].rectangle.width,
                    test_cases[i].rectangle.height,
                    monitor == NULL ? "(null)" : monitor->name);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    /* setup initial monitors */
    Monitor *monitor;
    Monitor *tail;
    for (unsigned i = 0; i < SIZE(monitors); i++) {
        monitor = xcalloc(1, sizeof(*monitor));
        monitor->name = xstrdup(monitors[i].name);
        monitor->x = monitors[i].rectangle.x;
        monitor->y = monitors[i].rectangle.y;
        monitor->width = monitors[i].rectangle.width;
        monitor->height = monitors[i].rectangle.height;
        monitor->frame = create_frame();
        if (Monitor_first == NULL) {
            Monitor_first = monitor;
        } else {
            tail->next = monitor;
        }
        tail = monitor;
    }

    add_test(monitor_directions);
    add_test(containing_frame);
    add_test(monitor_pattern);
    add_test(monitor_from_rectangle);
    return run_tests("Monitor");
}
