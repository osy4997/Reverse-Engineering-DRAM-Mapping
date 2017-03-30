#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define mmap_size 4194304 // 4MB

inline uint64_t time_stamp();
inline void clflush();
void Flush_Cache(uint64_t *first_PA, uint64_t *second_PA);
uint64_t latency_check(uint64_t *first_PA, uint64_t *second_PA);
int func();

int main()
{
    uint64_t first_PA, second_PA = 0;
    uint64_t gap = 0;
    int i = 0;
    for(i=0;i<mmap_size; i = i +1024)
    {

    }
    mmap(mmap_size);
    gap = latency_check(first_PA, second_PA);
    printf("%d", gap);
    
}

inline uint64_t time_stamp()
{
    unsigned hi, lo;
    int t;
    volatile void *p;
    asm volatile (
            "cpuid;"
            "rdtsc;"

            : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo)|(((unsigned long long)hi)<<32);
}
inline void clflush(volatile void *p)
{
    asm volatile ("clflush (%0)" :: "r"(p));
}
void Flush_Cache(uint64_t *first_PA, uint64_t *second_PA)
{
    clflush(first_PA);
    clflush(first_PA+64);
    clflush(first_PA+128);
    clflush(first_PA+192);
    clflush(first_PA+256);

    clflush(second_PA);
    clflush(second_PA + 64);
    clflush(second_PA + 128);
    clflush(second_PA + 192);
    clflush(second_PA + 256);

}
uint64_t latency_check(uint64_t *first_PA, uint64_t *second_PA)
{
    uint64_t start_f, end_f, start_s, end_s, gap_f, gap_s = 0;

    Flush_Cache();
    
    start_f = time_stamp();
    //////TODO///////////
    end_f = time_stamp();
    gap_f = end_f - start_f;

    Flush_Cache();

    start_s = time_stamp();
    //////TODO////////////
    end_s = time_stamp();
    gap_s = end_s - start_s;

    return gap_s - gap_f;
}

int func() {


    return 5;
}
