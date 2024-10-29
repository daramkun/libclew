#include "clew.h"

#if (__linux__ || __unix__) && !__ANDROID__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// #define _GNU_SOURCE <- define from CMakeLists.txt
#include <features.h>
#include <pthread.h>
#include <sched.h>

#if !__cplusplus
#define nullptr NULL
#endif

void determine_smt(const size_t core_count, bool* smt) {
    int i;
    char sibling[256], buffer[256];
    FILE* fp;

    for (i = 0; i < core_count; ++i) {
        snprintf(sibling, sizeof(sibling), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", i);
        fp = fopen(sibling, "r");
        if (!fp) {
            smt[i] = false;
            continue;
        }

        fgets(buffer, sizeof(buffer), fp);

        fclose(fp);
        fp = nullptr;
        
        smt[i] = strchr(buffer, ',') != nullptr;
    }
}

void determine_perf_level(const size_t core_count, int* perf_level) {
    int i, max_level = 0;
    char capacity[256], buffer[256];
    FILE* fp;

    for (i = 0; i < core_count; ++i) {
        snprintf(capacity, sizeof(capacity), "/sys/devices/system/cpu/cpu%d/cpu_capacity", i);
        fp = fopen(capacity, "r");
        if (!fp) {
            perf_level[i] = INT32_MAX;
            continue;
        }

        fgets(buffer, sizeof(buffer), fp);

        fclose(fp);
        fp = nullptr;

        perf_level[i] = atoi(buffer);
        if (max_level < perf_level[i]) {
            max_level = perf_level[i];
        }
    }

    for (i = 0; i < core_count; ++i) {
        if (perf_level[i] == INT32_MAX) {
            perf_level[i] = 0;
        } else {
            perf_level[i] = max_level - perf_level[i];
        }
    }
}

clew_cpu_info_t* clew_create()
{
    clew_cpu_info_t* cpu_info;
    size_t i;
    bool is_physical;
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
    cpu_set_t cpuset;
    int result, i;
    size_t cpu_count;

    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);

    thread = pthread_self();
    result = pthread_getaffinity_np(thread, sizeof(cpuset), &cpuset);
    if (result != 0)
        return 0xffffffffffffffff;

    for (i = 0; i < cpu_count; ++i) {
        if (CPU_ISSET(i, &cpuset)) {
            return i;
        }
    }

    return 0xffffffffffffffff;
}

void clew_set_thread_affinity(size_t core_index)
{
    pthread_t thread;
    cpu_set_t cpuset;

    thread = pthread_self();
    CPU_ZERO(&cpuset);
    CPU_SET(core_index, &cpuset);

    pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
}

#endif