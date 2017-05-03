/**
 * Minimal implementation of the `taskset` command that sets the CPU affinity of a new process.
 *
 * This implementation of `taskset` does not provide the full functionality of the `taskset` command
 * in `util-linux`.  Check if it fulfills your requirements before using this application.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>      // cpu_set_t, CPU_{SET,ZERO}(), sched_setaffinity()
#include <stdio.h>      // fprintf(), perror(), sscanf()
#include <unistd.h>     // access(), execl(), getpid()

void set_cpu_mask(cpu_set_t* const cpu_set, const unsigned mask)
{
    for (unsigned i_cpu = 0; i_cpu < 32; ++i_cpu) {
        const unsigned cpu_mask = 1 << i_cpu;
        if ((mask & cpu_mask) == cpu_mask)
            CPU_SET(i_cpu, cpu_set);
    }
}

int main(const int argc, const char* const* argv)
{
    int ret;

    if (argc < 2) {
        fprintf(stderr, "Please specify a valid hexadecimal CPU mask as first argument!\n");
        return -EINVAL;
    }

    unsigned cpu_mask = 0;
    ret = sscanf(argv[1], "%x", &cpu_mask);
    if (ret != 1) {
        fprintf(stderr, "Error: Failed to interpret '%s' as hexadecimal CPU mask!\n", argv[1]);
        return -EINVAL;
    }

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);

    set_cpu_mask(&cpu_set, cpu_mask);

    ret = sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);
    if (ret != 0) {
        perror("sched_setaffinity()");
        return ret;
    }

    if (argc < 3) {
        fprintf(stderr, "Please specify a valid binary as second argument!\n");
        return -EINVAL;
    }

    const char* const prog_path = argv[2];
    if (access(prog_path, F_OK) == -1) {
        fprintf(stderr, "Error: Could not access '%s'!\n", prog_path);
        return -EINVAL;
    }

    ret = execv(prog_path, (char* const*)&(argv[2]));
    if (ret != 0) {
        perror("execl()");
        return ret;
    }

    return 0;
}
