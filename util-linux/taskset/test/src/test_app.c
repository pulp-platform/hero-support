#define _GNU_SOURCE
#include <sched.h>      // sched_getaffinity()
#include <stdio.h>      // printf()
#include <unistd.h>     // getpid()

void print_cpu_set(const cpu_set_t* const cpu_set_p, const size_t n_cpus)
{
    for (size_t i_cpu = 0; i_cpu < n_cpus; ++i_cpu) {
        if (CPU_ISSET(i_cpu, cpu_set_p))
            printf("1");
        else
            printf("0");
    }
}

int main(int argc, const char* argv[])
{
    cpu_set_t set;
    CPU_ZERO(&set);

    const pid_t pid = getpid();

    int ret = sched_getaffinity(pid, sizeof(set), &set);
    if (ret != 0) {
        perror("sched_getaffinity");
        return ret;
    }

    printf("affinity:\t");
    print_cpu_set(&set, sizeof(set));
    printf("\n");

    printf("program:\t%s\n", argv[0]);

    printf("arguments:\t");
    for (int i = 1; i < argc; ++i)
        printf("%s, ", argv[i]);
    printf("\n");

    return 0;
}
