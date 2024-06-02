#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/personality.h>
#include <errno.h>
#include <capstone/capstone.h>

#include "misc.h"

// Funkcija za zagon debuggerja
static void run_debugger(pid_t pid)
{
    printf("Parent> Begin\n");

    int wstatus;
    waitpid(pid, &wstatus, 0); // Počakamo, da se child proces ustavi
    check_waitpid_status(wstatus);

    // Pridobimo začetni naslov pomnilnika procesa
    char path[100], temp[100];
    sprintf(path, "/proc/%d/maps", pid);
    int fd = open(path, O_RDONLY);
    read(fd, temp, 100);
    close(fd);

    char *del = strchr(temp, '-');
    *del = 0;
    unsigned long start_addr = strtol(temp, NULL, 16); // Pretvorimo naslov v število

    unsigned long addr = 0x11bf; // Privzeti naslov za breakpoint
    printf("Parent> Child start address: 0x%lx\n", start_addr);
    printf("Parent> Breakpoint address: 0x%lx\n", addr);

    // Preberemo podatke na danem naslovu in nastavimo breakpoint
    long data = ptrace(PTRACE_PEEKDATA, pid, start_addr + addr, NULL);
    long int_data = (data & ~0xff) | 0xcc; // Spremenimo zadnji bajt v 0xcc (int3)
    ptrace(PTRACE_POKEDATA, pid, start_addr + addr, int_data);
    ptrace(PTRACE_CONT, pid, NULL, NULL); // Nadaljujemo izvajanje child procesa
    waitpid(pid, NULL, 0); // Počakamo, da se child proces ustavi na breakpointu
    ptrace(PTRACE_POKEDATA, pid, start_addr + addr, data); // Obnovimo prvotne podatke

    struct user_regs_struct uregs;
    ptrace(PTRACE_GETREGS, pid, NULL, &uregs); // Pridobimo registre child procesa
    uregs.eip = start_addr + addr; // Nastavimo EIP register na naslov breakpointa
    ptrace(PTRACE_SETREGS, pid, NULL, &uregs);

    bool breakpoint_set = false;
    unsigned long breakpoint_addr;
    long original_data = 0;

    // Glavna zanka za vnos komand
    while (true)
    {
        printf("Command (b <addr> to set breakpoint, br to remove breakpoint, s to step, c to continue, p <addr> to print value, q to quit): ");
        char command[3];
        scanf("%2s", command);

        if (strcmp(command, "b") == 0 && !breakpoint_set)
        {
            // Nastavimo breakpoint na dani naslov
            printf("Parent> Enter breakpoint address (hex): ");
            scanf(" %lx", &breakpoint_addr);
            unsigned long offset = breakpoint_addr - start_addr;
            original_data = ptrace(PTRACE_PEEKDATA, pid, start_addr + offset, NULL);
            long new_int_data = (original_data & ~0xff) | 0xcc;
            ptrace(PTRACE_POKEDATA, pid, start_addr + offset, new_int_data);
            breakpoint_set = true;
            printf("Parent> Breakpoint set at: 0x%lx\n", breakpoint_addr);
        }
        else if (strcmp(command, "br") == 0 && breakpoint_set)
        {
            // Odstranimo breakpoint
            unsigned long offset = breakpoint_addr - start_addr;
            ptrace(PTRACE_POKEDATA, pid, start_addr + offset, original_data);
            breakpoint_set = false;
            printf("Parent> Breakpoint removed\n");
        }
        else if (strcmp(command, "c") == 0)
        {
            // Nadaljujemo izvajanje in preverimo ali smo dosegli breakpoint
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            waitpid(pid, &wstatus, 0);
            if (WIFEXITED(wstatus))
            {
                printf("Parent> Child exited\n");
                break;
            }
            if (WIFSTOPPED(wstatus) && WSTOPSIG(wstatus) == SIGTRAP)
            {
                printf("Parent> Breakpoint hit at: 0x%lx\n", breakpoint_addr);
                ptrace(PTRACE_GETREGS, pid, NULL, &uregs);
                unsigned long i_addr = uregs.ebp - 0xc;
                long i_val = ptrace(PTRACE_PEEKDATA, pid, i_addr, NULL);
                printf("Parent> Value of i: %ld\n", i_val);
                ptrace(PTRACE_POKEDATA, pid, breakpoint_addr, original_data);
                uregs.eip = breakpoint_addr;
                ptrace(PTRACE_SETREGS, pid, NULL, &uregs);
                ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL); // Enkratno izvedemo naslednjo instrukcijo
                waitpid(pid, &wstatus, 0);
                if (WIFEXITED(wstatus))
                {
                    printf("Parent> Child exited\n");
                    break;
                }
                breakpoint_set = false;
            }
        }
        else if (strcmp(command, "s") == 0)
        {
            // En korak naprej
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
            waitpid(pid, &wstatus, 0);
            if (WIFEXITED(wstatus))
            {
                printf("Parent> Child exited\n");
                return;
            }
            ptrace(PTRACE_GETREGS, pid, NULL, &uregs);
            printf("Parent> Stepped to eip: 0x%lx\n", uregs.eip);
        }
        else if (strcmp(command, "p") == 0)
        {
            // Izpis vrednosti na danem naslovu
            unsigned long print_addr;
            printf("Parent> Enter address to print value (hex): ");
            scanf(" %lx", &print_addr);
            long value = ptrace(PTRACE_PEEKDATA, pid, print_addr, NULL);
            printf("Parent> Value at address 0x%lx: %ld\n", print_addr, value);
        }
        else if (strcmp(command, "q") == 0)
        {
            // Izhod iz debuggerja
            printf("Parent> Quitting\n");
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            break;
        }
        else
        {
            printf("Parent> Unknown command\n");
        }
    }

    printf("Parent> End\n");
}

// Funkcija za zagon child procesa
static void run_child()
{
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    char *c_pathname = "./debuggee";
    char *c_argv[] = {c_pathname, NULL};
    execve(c_pathname, c_argv, NULL);
}

// Glavna funkcija, kjer forkanje in zagon child procesa
int main()
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(-1);
    }
    else if (pid == 0)
    {
        personality(ADDR_NO_RANDOMIZE);
        run_child();
    }
    else
    {
        run_debugger(pid);
    }
    return 0;
}
