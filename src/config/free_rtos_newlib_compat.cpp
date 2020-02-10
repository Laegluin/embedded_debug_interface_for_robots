#include <FreeRTOS.h>
#include <malloc.h>
#include <string.h>
#include <task.h>

extern "C" void* __wrap__malloc_r(_reent*, size_t size) {
    return pvPortMalloc(size);
}

// required because newlib's implementation breaks with a custom malloc
extern "C" void* __wrap__calloc_r(_reent*, size_t num, size_t size) {
    void* mem = pvPortMalloc(num * size);
    if (!mem) {
        return nullptr;
    }

    memset(mem, 0, num * size);
    return mem;
}

extern "C" void __wrap__free_r(_reent*, void* ptr) {
    vPortFree(ptr);
}
