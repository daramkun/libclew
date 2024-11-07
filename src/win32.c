#include "clew.h"

#if WIN32 || WINDOWS || _WINDOWS

#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include <VersionHelpers.h>

#if !__cplusplus
#define nullptr NULL
#endif

size_t get_core_index(KAFFINITY* affinity)
{
    size_t i = 0;
    while (!(*affinity & (1 << i))) {
        ++i;

        if (i == 0)
            return 0xffffffff;
    }

    *affinity = ~(~*affinity | (1 << i));
    return i;
}

clew_cpu_info_t* clew_create()
{
    clew_cpu_info_t* cpu_info;
    SYSTEM_INFO system_info;

    DWORD logical_processor_information_length = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info_buffer = nullptr, *info = nullptr;
    DWORD i, index;
    KAFFINITY affinity;
    bool is_physical;

    cpu_info = (clew_cpu_info_t*)malloc(sizeof(clew_cpu_info_t));

    GetSystemInfo(&system_info);
    cpu_info->core_count = system_info.dwNumberOfProcessors;
    cpu_info->maximum_efficiency_level = 0;

    cpu_info->cores = (clew_core_info_t*)malloc(sizeof(clew_core_info_t) * cpu_info->core_count);

    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &logical_processor_information_length);
    info_buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)_alloca(logical_processor_information_length);
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, info_buffer, &logical_processor_information_length))
    {
        free(cpu_info->cores);
        free(cpu_info);

        return nullptr;
    }

    for (i = 0; i < logical_processor_information_length;) {
        info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((BYTE*)info_buffer) + i);

        if (info->Relationship != RelationProcessorCore)
            continue;

        affinity = info->Processor.GroupMask[0].Mask;

        while (info->Processor.GroupMask[0].Mask) {
            is_physical = info->Processor.GroupMask[0].Mask == affinity || !(info->Processor.Flags & LTP_PC_SMT);
            index = get_core_index(&info->Processor.GroupMask[0].Mask);

            cpu_info->cores[index].core_index = index;
            cpu_info->cores[index].is_smt_core = !is_physical;
#ifdef _MSC_VER
            if (IsWindows10OrGreater()) {
                cpu_info->cores[index].efficiency_level = (uint8_t)info->Processor.EfficiencyClass;
            } else {
                cpu_info->cores[index].efficiency_level = 0;
            }
#else
            // No support `EfficiencyClass` in MinGW
            cpu_info->cores[index].efficiency_level = 0;
#endif

            if (cpu_info->maximum_efficiency_level < cpu_info->cores[index].efficiency_level)
                cpu_info->maximum_efficiency_level = cpu_info->cores[index].efficiency_level;
        }

        i += info->Size;
    }

    if (cpu_info->maximum_efficiency_level != 0) {
        for (i = 0; i < cpu_info->core_count; ++i) {
            cpu_info->cores[i].efficiency_level = cpu_info->maximum_efficiency_level - cpu_info->cores[i].efficiency_level;
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
    GROUP_AFFINITY  affinity;
    HANDLE current_thread = GetCurrentThread();
    GetThreadGroupAffinity(current_thread, &affinity);

    return get_core_index(&affinity.Mask);
}

void clew_set_thread_affinity(size_t core_index)
{
    HANDLE current_thread = GetCurrentThread();
    SetThreadAffinityMask(current_thread, (1 << core_index));
}

#endif