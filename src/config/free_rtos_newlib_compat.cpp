#include <FreeRTOS.h>
#include <malloc.h>
#include <task.h>

extern "C" void* __wrap__malloc_r(_reent*, size_t size) {
    return pvPortMalloc(size);
}

extern "C" void __wrap__free_r(_reent*, void* ptr) {
    vPortFree(ptr);
}
