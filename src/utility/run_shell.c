#include <stdlib.h> /* malloc() */
#include <string.h> /* memchr(), memcpy() */
#include <sys/wait.h> /* waitpid() */
#include <unistd.h> /* fork(), setsid(), _exit(), execl(), read(), close() */

/* Run @command within a shell in the background. */
int run_shell(const char *command)
{
    pid_t child_process_id;

    /* Using `fork()` twice and `_exit()` will give the child process to the
     * init system.  Then we do not need to worry about cleaning up dead child
     * processes.  `setsid()` is used to make a new session to completely
     * isolate the child process.
     *
     * `_exit()` is used for child processes.
     */

    /* create a child process */
    child_process_id = fork();
    if (child_process_id == -1) {
        return 1;
    }

    if (child_process_id == 0) {
        /* this code is executed in the child */

        /* create a grandchild process */
        if (fork() == 0) {
            /* this code is executed in the grandchild process */

            /* make a new session */
            if (setsid() == -1) {
                /* EPERM: The calling process is already a process group
                 * leader.
                 */
                _exit(EXIT_FAILURE);
            }

            (void) execl("/bin/sh", "sh", "-c", command, (char*) NULL);
            /* this point is only reached if `execl()` failed */
            _exit(EXIT_FAILURE);
        } else {
            /* exit the child process */
            _exit(EXIT_SUCCESS);
        }
    } else {
        /* wait until the child process exits */
        (void) waitpid(child_process_id, NULL, 0);
    }

    return 0;
}

/* Run @command as command within a shell and get the first line from it. */
char *run_shell_and_get_output(const char *command)
{
    pid_t child_process_id;
    int pipe_descriptors[2];
    char buffer[1024];
    ssize_t count;
    char *output;

    /* open a pipe */
    if (pipe(pipe_descriptors) < 0) {
        return NULL;
    }

    child_process_id = fork();
    switch (child_process_id) {
    /* fork failed */
    case -1:
        close(pipe_descriptors[0]);
        close(pipe_descriptors[1]);
        return NULL;

    /* child process */
    case 0:
        close(pipe_descriptors[0]);
        if (pipe_descriptors[1] != STDOUT_FILENO) {
            dup2(pipe_descriptors[1], STDOUT_FILENO);
            close(pipe_descriptors[1]);
        }
        (void) execl("/bin/sh", "sh", "-c", command, (char*) NULL);
        _exit(EXIT_FAILURE);
        break;
    }

    /* parent process */
    close(pipe_descriptors[1]);

    count = read(pipe_descriptors[0], buffer, sizeof(buffer) - 1);

    close(pipe_descriptors[0]);

    if (count < 0) {
        return NULL;
    }

    char *const new_line = memchr(buffer, '\n', count);
    if (new_line != NULL) {
        count = new_line - buffer;
    }

    output = malloc(count + 1);
    if (output == NULL) {
        return NULL;
    }

    memcpy(output, buffer, count);
    output[count] = '\0';
    return output;
}
