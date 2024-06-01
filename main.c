#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void run_debugger(pid_t child_pid) {
    if (ptrace(PTRACE_ATTACH, child_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        exit(EXIT_FAILURE);
    }
    // Wait for the child process to stop
    if (waitpid(child_pid, NULL, 0) == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    printf("Debugger: Attached to child process. PID: %d\n", child_pid);
    printf("Debugging: press 's' to step once, 'c' to continue, 'q' to quit\n");

    char command;
    while (scanf(" %c", &command) == 1) {
        if (command == 's') {
            // Use PTRACE_SINGLESTEP
            if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) == -1) {
                perror("ptrace singlestep");
                exit(EXIT_FAILURE);
            }
            if (waitpid(child_pid, NULL, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            printf("Stepped\n");
        } else if (command == 'c') {
            // Use PTRACE_CONT to continue execution
            if (ptrace(PTRACE_CONT, child_pid, NULL, NULL) == -1) {
                perror("ptrace continue");
                exit(EXIT_FAILURE);
            }
            if (waitpid(child_pid, NULL, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            printf("Continued\n");
        } else if (command == 'q') {
            if (ptrace(PTRACE_DETACH, child_pid, NULL, NULL) == -1) {
                perror("ptrace detach");
                exit(EXIT_FAILURE);
            }
            printf("Detached and exiting\n");
            break;
        } else {
            printf("Debugger: Unknown command. Type 'c' to continue, 's' to step, 'q' to quit.\n");
        }
    }
}

void run_debuggee() {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        perror("ptrace traceme");
        exit(EXIT_FAILURE);
    }
    printf("Debuggee: Started. PID: %d\n", getpid());
    fflush(stdout); // Flush output
    for (int i = 0; i < 10; ++i) {
        printf("Debuggee: Iteration %d\n", i);
        fflush(stdout); // Flush output
        sleep(1);
    }
    printf("Debuggee: Finished.\n");
    fflush(stdout); // Flush output
}

int main() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        run_debuggee();
    } else {
        run_debugger(pid);
    }

    return 0;
}
