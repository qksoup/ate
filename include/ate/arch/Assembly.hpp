#ifndef ATE_ASSEMBLY_HPP
#define ATE_ASSEMBLY_HPP

namespace ate {
    // swap *mem with val and return old value of *mem
    inline int testAndSet(volatile int* mem);
    inline int cmpAndSwap(volatile int* mem, int cmpVal, int newVal);
    inline void cpuPause();    
    inline void cpuPause(int delay); 
    inline void barrier();
    inline void rmembar();
    inline void wmembar();
    inline void membar();
    inline uint64_t getClockTicks();
} // namespace ate

// include internal header file for platform specific implementaion
#ifdef __i686__
#include <ate/arch/Assembly_i686.hpp>

#elif defined(__i386__)
#include <ate/arch/Assembly_i386.hpp>

#elif defined(__x86_64__)
#include <ate/arch/Assembly_x86_64.hpp>

#elif defined(__ia_64__)
#error "ate Atomic operations for IA64 have not been implemented"

#else
#error "this platform is not supported"
#endif    

#endif // ATE_ASSEMBLY_HPP
