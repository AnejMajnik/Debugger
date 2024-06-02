#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>

#include "misc.h"

static void run_debugger(pid_t child_pid) {
    printf("Debugger> Begin\n");
    
    int wstatus;
    waitpid(child_pid, &wstatus, 0);
    check_waitpid_status(wstatus);

    printf("Debugee start in 2 seconds\n");
    sleep(2);

    long rr = ptrace(PTRACE_CONT, child_pid, 0, 0);
    check_ptrace(rr);

    waitpid(child_pid, NULL, 0);

    printf("Debugger> End\n");

}

static void run_debuggee() {
    long r = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	check_ptrace(r);

    char *c_pathname = "./debuggee";
    char *c_argv[] = {c_pathname, NULL};
    char *c_envp[] = {NULL};

    int rr = execve(c_pathname, c_argv, c_envp);
    check_execve(rr);
}

int main()
{
	// Ustvari nov proces
	pid_t pid = fork();

	switch (pid)
	{
		case -1:
			// Napaka
			perror("fork");
			exit(-1);
			break;
		case 0:
			// Otrok (child process)
			run_debuggee();
			break;
		default:
			// Stars (parent process)
			run_debugger(pid);
			break;
	}

	return 0;
}