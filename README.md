## libclew
`libclew` is CPU efficiency level and SMT information determining library.

### Usage
```cpp
#include <clew.h>

int main(int argc, char* argv[]) {
    clew_cpu_info_t* cpu_info = clew_create();
    
    for (auto i = 0; i < cpu_info->core_count; ++i) {
        const auto current_core = cpu_info->cores[i];
        ...
    }
    
    clew_destroy(cpu_info);
    cpu_info = nullptr;
}
```