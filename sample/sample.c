#include <stdio.h>
#include <clew.h>

int main(int argc, char *argv[])
{
    clew_cpu_info_t* cpu_info;
    size_t i = 0;

    cpu_info = clew_create();

    printf("core count: %lld\n", cpu_info->core_count);
    printf("maximum efficiency level: %d\n", cpu_info->maximum_efficiency_level);

    for (i = 0; i < cpu_info->core_count; ++i) {
        printf("%2lld core: smt?: %c, efficiency_level: %d\n",
               cpu_info->cores[i].core_index,
               cpu_info->cores[i].is_smt_core ? 'T' : 'F',
               cpu_info->cores[i].efficiency_level
        );
    }

    clew_destroy(cpu_info);

    return 0;
}