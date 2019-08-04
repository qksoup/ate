#ifndef ATE_ASSEMBLY_HPP
#error "Do not include this file directly; include ate/Assembly.hpp instead"
#else 

namespace ate {
    inline int testAndSet(volatile int* mem)
    {
        int prev;
        __asm__ volatile ("movl $1, %%eax\n\t"
                          "lock xchgl %%eax, (%%ecx)"
                          : "=a" (prev)
                          : "c" (mem)
                          : "memory");
        if (prev != 0 && prev != 1)
            ATE_ABORT("prev = " << prev);
        return prev;               
    }
    
    inline int cmpAndSwap(volatile int* mem, int cmpVal, int newVal)
    {
        int prev;
        __asm__ volatile ("lock cmpxchgl %1, %2"
                          : "=a" (prev)
                          : "r" (newVal), "m" (*mem), "0" (cmpVal)
                          : "memory" );
        
        return prev;
        
    }

    inline void cpuPause()
    {
        __asm__ volatile ("pause;");
    }

    inline void cpuPause(int delay) 
    {
        for (int i = 0; i < delay; ++i) {
            __asm__ volatile ("pause;");
        }
    }
    
    inline void barrier()
    {
        __asm__ volatile ("" : : : "memory" );
    }
    
    inline void rmembar()
    {
        __asm__ volatile ("" : : : "memory" );
    }
    
    inline void wmembar()
    {
        __asm__ volatile ("" : : : "memory" );
    }
    
    inline void membar()
    {
        __asm__ volatile ("" : : : "memory" );
    }
    
    inline uint64_t getClockTicks()
    {        
        unsigned int a, d;
        __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
        return ((uint64_t)a) | (((uint64_t)d) << 32);;
    }


} // namespace ate 

#endif
