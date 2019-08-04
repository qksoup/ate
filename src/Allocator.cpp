#include <ate/ate.hpp>

namespace ate {
    /// linux page size is 4096 bytes.
    /// malloc() uses 8 bytes for x86 and 16 bytes for x86_64. 
#ifdef __i386__    
    const size_t Bin::s_size = 4 * 4096 - sizeof(ate::Region::Link) - 8;
#else
    const size_t Bin::s_size = 4 * 4096 - sizeof(ate::Region::Link) - 16;
#endif

    Region::Region() : m_head(NULL) {}
    
    Region::~Region()
    {
        for (Link* cur = m_head; cur != NULL; cur = m_head) {
            m_head = m_head->next;
            ::free(cur);
        }
    }
    
    void* Region::malloc(size_t n) 
    {        
        Link* link = static_cast<Link*>(::malloc(sizeof(Link) + n));
        if (link == NULL) {
            std::cerr << "malloc() out of memory!!" << std::endl;
            ::abort();
        }
        link->next = m_head;
        m_head = link;
        return (link + 1);
    }

    Bin::Bin()
    {
        m_cur = static_cast<char*>(m_region.malloc(s_size));
        m_end = m_cur + s_size;
    }

    void* Bin::alloc(size_t n, bool zero)
    {
        n = align(n);
        char* ptr;
        if (n > s_size) { // big memory request
            ptr = static_cast<char*>(m_region.malloc(n));
        } else {
            if (m_cur + n > m_end) { // discard what is left in this page
                m_cur = static_cast<char*>(m_region.malloc(s_size));
                m_end = m_cur + s_size;
            }            
            ptr = m_cur;
            m_cur += n;
        }
        
        if (zero)
            memset(ptr, 0, n);
        return ptr;
    }

    char* Bin::strdup(const char* str)
    {
        size_t size = strlen(str) + 1;
        char* dst = static_cast<char*>(this->alloc(size, false));
        strncpy(dst, str, size);
        return dst;
    }

    Bin::~Bin() {}

} // namespace ate
