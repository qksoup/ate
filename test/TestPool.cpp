#include <ate/ate.hpp>

// C include
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
// C++ include
#include <iostream>

namespace {
    struct Junk {
        int i;
        double d;
        bool b;
        char str[128];
        long l;
        float f;
    };

}

int main() 
{
    using namespace ate;
    using namespace std;

    {
        Junk* junks[1024];
        Pool<Junk> pool;
        for (int i = 0; i < 1024; ++i) {
            for (int j = 0; j < 1024; ++j) {
                Junk* junk = pool.alloc();
                junks[j] = junk;
            }
            for (int j = 0; j < 1024; j += 2) {
                pool.dealloc(junks[j]);
            }
            for (int j = 1; j < 1024; j += 2) {
                pool.dealloc(junks[j]);
            }
            //cout << i << ": allocate() and deallocate()" << endl;
        }
    }

    Bin bin;
    Junk* junk = bin.create<Junk>();
    assert(junk->i == 0 && junk->f == 0.0);
    const char* str = "hello, world";
    char* str2;
    void* ptr = bin.alloc(4098);
    memset(ptr, 0, 4098);
    for (int i = 0; i < 10000; ++i) {
        str2 = bin.strdup(str);
        if (strcmp(str, str2) != 0)
            ::abort();
        if (i % 100 == 99)
            cout << "bin.strdup() " << i + 1 << " times" << endl;
    }
}
