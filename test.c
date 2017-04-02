#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#define mmap_size 4096//4194304 // 4MB
#define page_size 4096// 4KB
#define total_size mmap_size * mmap_size * 4
#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

int i, c, pid, status;
unsigned long virt_addr; 
uint64_t read_val, file_offset;
char path_buf [0x100] = {};
FILE * f;
char *end;

inline uint64_t time_stamp();
inline void clflush();
void Flush_Cache(char *first_PA, char *second_PA);
int latency_check(char *first_PA, char *second_PA);
int func();
int read_pagemap(char * path_buf, unsigned long virt_addr);
uint64_t Check_Gap(uint64_t a, uint64_t b);

int main()
{
    char *first_PA, *second_PA ;
    uint64_t gap = 0;
    int i,j,k = 0;
    char test;

    int PA = 0;
    first_PA = (char *)calloc(0,total_size);//virtual address TODO
    second_PA = first_PA;


    pid = getpid();
    printf("pid : %d\n",pid);
    sprintf(path_buf, "/proc/%u/pagemap", pid);
    //for(i=0;i<total_size; i++)
    //    memset(second_PA++, '1', 1);


    for(i=0;i<total_size; i = i + mmap_size)
    {
    //    memset(first_PA, '1', 1);
        printf("%d.",++j);
        for(k=0;k<4096;k++)
            printf("%c", *(first_PA+k));


        read_pagemap(path_buf, (unsigned long)first_PA);

        printf("virtual address : %p, ", first_PA);
        printf("physical address : 0x%llx\n", (unsigned long long) read_val);



        first_PA = first_PA + mmap_size;


//        gap = latency_check(first_PA, second_PA);
//        printf("-time gap\n");
//        second_PA += page_size;
        sleep(1);



    }
    sleep(1000000);


}

inline uint64_t time_stamp()
{
    unsigned hi, lo;
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
void Flush_Cache(char *first_PA, char *second_PA)
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
int latency_check(char *first_PA, char *second_PA)
{
    uint64_t start_f, end_f, start_s, end_s, gap_f, gap_s = 0;
    char value;
    char *address;
    address = (char*)time_stamp();
    Flush_Cache(first_PA, second_PA);
 
    start_f = time_stamp();
    value = *first_PA;
    end_f = time_stamp();
   
    gap_f = Check_Gap(end_f, start_f);


//    printf("first operation : %p / %" PRIu64, first_PA, gap_f);
   
    address = (char*)time_stamp();
    Flush_Cache(first_PA, second_PA);

    start_s = time_stamp();
    value = *second_PA;
    end_s = time_stamp();
   
    gap_s = Check_Gap(end_s, start_s); 

//    printf(" ,  second operation : %p / %" PRIu64, second_PA, gap_s);

    return Check_Gap(gap_f, gap_s);
}

int func() {


    return 5;
}
uint64_t Check_Gap (uint64_t a, uint64_t b)
{
    if (a>b)
        return a-b;
    else
        return b-a;
}
int read_pagemap(char * path_buf, unsigned long virt_addr)
{
    //printf("Big endian? %d\n", is_bigendian());
    f = fopen(path_buf, "rb");
    if(!f){
        printf("Error! Cannot open %s\n", path_buf);
        return -1;
    }

    //Shifting by virt-addr-offset number of bytes
    //and multiplying by the size of an address (the size of an entry in pagemap file)
    file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
    //printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", virt_addr, getpagesize(), PAGEMAP_ENTRY);
    //printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);
    status = fseek(f, file_offset, SEEK_SET);
    if(status){
        perror("Failed to do fseek!");
        return -1;
    }
    errno = 0;
    read_val = 0;
    unsigned char c_buf[PAGEMAP_ENTRY];
    for(i=0; i < PAGEMAP_ENTRY; i++){
        c = getc(f);
        if(c==EOF){
            printf("\nReached end of the file\n");
            return 0;
        }
        if(is_bigendian())
            c_buf[i] = c;
        else
            c_buf[PAGEMAP_ENTRY - i - 1] = c;
        //printf("[%d]0x%x ", i, c);
    }
    for(i=0; i < PAGEMAP_ENTRY; i++){
        //printf("%d ",c_buf[i]);
        read_val = (read_val << 8) + c_buf[i];
    }
    //printf("\n");
    //printf("Result: 0x%llx\n", (unsigned long long) read_val);
    //if(GET_BIT(read_val, 63))
    if(GET_BIT(read_val, 63))
        printf("PFN: 0x%llx\n",(unsigned long long) GET_PFN(read_val));
    else
        printf("Page not present\n");
    if(GET_BIT(read_val, 62))
        ;
        //printf("Page swapped\n");
    fclose(f);
    return 0;
}
