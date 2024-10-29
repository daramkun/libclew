#ifndef __LIBCLEW_H__
#define __LIBCLEW_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if _WIN32 || _WIN64 || _WINDOWS || WINDOWS
#if defined(CLEW_DLL_EXPORT)
#define CLEWEXP             __declspec(dllexport)
#else
#define CLEWEXP             __declspec(dllimport)
#endif
#else
#define CLEWEXP
#endif

#if __cplusplus
constexpr uint8_t efficiency_level_maximum_performance = 0;
#else
#define efficiency_level_maximum_performance 0
#endif

struct clew_core_info {
    size_t core_index;
    bool is_smt_core;
    uint8_t efficiency_level;
};

#if __cplusplus
using clew_core_info_t = clew_core_info;
#else
typedef struct clew_core_info clew_core_info_t;
#endif

struct clew_cpu_info {
    clew_core_info_t* cores;
    size_t core_count;
    uint8_t maximum_efficiency_level;
};

#if __cplusplus
using clew_cpu_info_t = clew_cpu_info;
#else
typedef struct clew_cpu_info clew_cpu_info_t;
#endif

#if __cplusplus
extern "C" {
#endif

CLEWEXP clew_cpu_info_t* clew_create();
CLEWEXP void clew_destroy(clew_cpu_info_t*);

CLEWEXP size_t clew_get_thread_affinity();
CLEWEXP void clew_set_thread_affinity(size_t core_index);

#if __cplusplus
};
#endif

#endif