#include "clew.h"

#if __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>

#if !__cplusplus
#define nullptr NULL
#endif

void determine_perf_level(const size_t core_count, int* perf_level) {
#if __arm64 || __arm64__ || __aarch64__
    size_t length = sizeof(int);
    int maximum_perf_level, *perf_levels;
    int i, j = 0, k = 0;
    char sysctl_name[64];

    if (sysctlbyname("hw.nperflevels", &maximum_perf_level, &length, nullptr, 0) != 0) {
        for (i = 0; i < core_count; ++i) {
            perf_level[i] = efficiency_level_maximum_performance;
        }

        return;
    }

    perf_levels = (int*)alloca(maximum_perf_level * sizeof(int));
    for (i = maximum_perf_level - 1; i >= 0; --i) {
        snprintf(sysctl_name, sizeof(sysctl_name), "hw.perflevel%d.logicalcpu", i);
        if (sysctlbyname(sysctl_name, &perf_levels[i], &length, nullptr, 0) != 0) {
            continue;
        }

        for (j = 0; j < perf_levels[i]; ++j) {
            perf_level[k + j] = maximum_perf_level - i - 1;
        }

        k += perf_levels[i];
    }
#elif __i386 || __x86_64
    int i;

    for (i = 0; i < core_count; ++i) {
        perf_level[i] = 0;
    }
#endif
}

void determine_smt(const size_t core_count, bool* smt) {
#if __arm64 || __arm64__ || __aarch64__
    int i;

    for (i = 0; i < core_count; ++i) {
        smt[i] = false;
    }
#elif __i386 || __x86_64
    int i;
    size_t length = sizeof(int);
    int physical_count, logical_count;

    sysctlbyname("machdep.cpu.core_count", &physical_count, &length, nullptr, 0);
    sysctlbyname("machdep.cpu.thread_count", &logical_count, &length, nullptr, 0);

    if (physical_count < logical_count) {
        for (i = 0; i < core_count; ++i) {
            smt[i] = i % 2 != 0;
        }
    } else {
        for (i = 0; i < core_count; ++i) {
            smt[i] = false;
        }
    }
#endif
}

clew_cpu_info_t* clew_create()
{
    clew_cpu_info_t* cpu_info;
    size_t i;
    int* perf_level;
    bool* smt;

    cpu_info = (clew_cpu_info_t*)malloc(sizeof(clew_cpu_info_t));

    cpu_info->core_count = sysconf(_SC_NPROCESSORS_ONLN);
    cpu_info->maximum_efficiency_level = 0;

    perf_level = (int*)alloca(sizeof(int) * cpu_info->core_count);
    determine_perf_level(cpu_info->core_count, perf_level);

    smt = (bool*)alloca(sizeof(bool) * cpu_info->core_count);
    determine_smt(cpu_info->core_count, smt);

    cpu_info->cores = (clew_core_info_t*)malloc(sizeof(clew_core_info_t) * cpu_info->core_count);

    for (i = 0; i < cpu_info->core_count; ++i) {
        cpu_info->cores[i].core_index = i;
        cpu_info->cores[i].efficiency_level = perf_level[i];
        cpu_info->cores[i].is_smt_core = smt[i];
    }

    for (i = 0; i < cpu_info->core_count; ++i) {
        if (cpu_info->maximum_efficiency_level < cpu_info->cores[i].efficiency_level) {
            cpu_info->maximum_efficiency_level = cpu_info->cores[i].efficiency_level;
        }
    }

    return cpu_info;
}

void clew_destroy(clew_cpu_info_t* cpu_info)
{
    if (cpu_info == nullptr)
        return;

    if (cpu_info->cores == nullptr) {
        free(cpu_info);
        return;
    }

    free(cpu_info->cores);
    free(cpu_info);
}

size_t clew_get_thread_affinity()
{
    pthread_t thread;
    mach_port_t mach_thread;
    int32_t policy, count = THREAD_AFFINITY_POLICY_COUNT;

    thread = pthread_self();
    mach_thread = pthread_mach_thread_np(thread);

    if (thread_policy_get(mach_thread, THREAD_AFFINITY_POLICY,
        &policy, (mach_msg_type_number_t*)&count, nullptr) != KERN_SUCCESS)
        return 0xffffffffffffffff;

    return policy;
}

void clew_set_thread_affinity(size_t core_index)
{
    pthread_t thread;
    mach_port_t mach_thread;
    thread_affinity_policy_data_t policy = { (int)core_index };

    thread = pthread_self();
    mach_thread = pthread_mach_thread_np(thread);

    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
}

#endif