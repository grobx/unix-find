#include <dlfcn.h>
#include <iostream>

extern "C" void* malloc(size_t size) {
    static auto real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    void* ptr = real_malloc(size);
    std::cerr << "malloc(" << size << ") -> " << ptr << std::endl;
    return ptr;
}

extern "C" void free(void* ptr) {
    static auto real_free = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    std::cerr << "free(" << ptr << ")" << std::endl;
    real_free(ptr);
}