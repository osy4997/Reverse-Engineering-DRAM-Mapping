#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#define mmap_size 1//4194304 // 4MB
#define page_size 4096// 4KB
#define total_size page_size * page_size * 64
#define list_size total_size / page_size
#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
#define sample 200

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

int i, c, pid, status;
unsigned long virt_addr; 
uint64_t read_val, file_offset;
char path_buf [0x100] = {};
FILE * f;
char *end;

typedef struct table_entry{
    uint64_t PFN;
    uint64_t VA;
} table_entry;
typedef struct pair{
    table_entry first;
    table_entry second;
    int diff_bit;
    int latency;
} pair;
pair test_pair[sample];
table_entry PT[list_size];
inline uint64_t time_stamp();
inline void clflush();
void Flush_Cache(char *first_PA, char *second_PA);
int gap_check(char *first_PA, char *second_PA);
int PTN;
int func();
int read_pagemap(char * path_buf, unsigned long virt_addr);
uint64_t Check_Gap(uint64_t a, uint64_t b);
int make_table();
void sort(int low, int high);
int partition (int low, int high);
void print();
void concheck();
void latency_check();
int test();
void select_();
void dec2bin(int a, char *bin);
int diff(uint64_t a, uint64_t b);
void test_push(int i,int j,int bit);
int test_size();
void diff_swap(int a, int b);

int main()
{

    int input;
    table_entry table_entry[list_size];

    while(1)
    {
        printf("1. make page table \n2. sort\n3. print\n4. latency check\n5. test\n");
        scanf("%d",&input);
        switch (input){
            case 1:
                make_table();
                //printf("final PFN : %"PRIu64"\n",table_entry[0].PFN);
                //printf("final virt : %"PRIu64"\n", table_entry[0].virt_addr);
                break;
            case 2:
                sort(0,list_size-1);
                break;
            case 3:
                print();
                break;
            case 4:
                //latency_check();
                printf("time %"PRIu64"\n", time_stamp());
                break;
            case 5:
                test();
                break;
            default:
                printf("retype\n");
                break;
        }
    }

    return 0;
}
int test()
{
    int i,j;
    int start,end;

    select_();
    char a,b;
    char *c;
    char *d;
    int size = test_size();
    
    for(i = 0 ; i < size ; i++)
    {
//        printf("%d.",i+1);
//        printf("first : %x\n", (unsigned int) test_pair[i].first.PFN);
//        printf("second : %x\n", (unsigned int) test_pair[i].second.PFN);
//        printf("different bit : %d\n", test_pair[i].diff_bit);

        c = (char *)(test_pair[i].first.VA);
        d = (char *)(test_pair[i].second.VA);
        start = time_stamp();
        for(j=0;j<1000;j++)
        {
            a = *c;
            b = *d;

            asm volatile
                (
                 "clflush (%0);"
                 "clflush (%1);"
                 "mfence;"
                 :: "q"(c), "p"(d)
                );
        }
        end = time_stamp();
        test_pair[i].latency = (end-start)/1000;
    }

    for(i = 0; i<sample; i++)
    {
        printf("%d. different bit: %d,  latency: %d\n",i,test_pair[i].diff_bit,test_pair[i].latency);
    }

  
    return (start-end)/1000;
}
int test_size()
{
    int i;
    int size = 0;
    for(i = 0 ; i< sample; i++)
    {
//        if(test_PT[i][0].VA == 0)
        if(test_pair[i].first.VA == 0)
            break;
        size++;
    }
    printf("size : %d\n",size);
    return size;
}
void select_()
{
    int i,j,k = 0;
    int bit;

    for(i = 0; i< 200; i++)
    {
        for(j=i+1; j<list_size; j++)
        {
            bit = diff(PT[i].PFN,PT[j].PFN);
            if(bit != -1)//differnet only one bit
            {
                test_push(i,j,bit);
                break;
            }
            if(j == list_size - 1)//not found
            {
                break;
            }
        }
        if(test_size() == sample)
            break;
    }
    sort(0, sample-1);
    return ;
}
void test_push(int i, int j, int bit)
{
    int size = test_size();
//    test_PT[size][0].VA = PT[i].VA;
//    test_PT[size][0].PFN = PT[i].PFN;
//    test_PT[size][1].VA = PT[j].VA;
//    test_PT[size][1].PFN = PT[j].PFN;

    test_pair[size].first.VA = PT[i].VA;
    test_pair[size].first.PFN = PT[i].PFN;
    test_pair[size].second.VA = PT[j].VA;
    test_pair[size].second.PFN = PT[j].PFN;
    test_pair[size].diff_bit = bit;





    return ;


}
int diff(uint64_t a, uint64_t b)
{
    char buffer_a[64] = {'0'};
    char buffer_b[64] = {'0'};
    int i;
    int dif = 0;
    dec2bin((int) a, buffer_a);
    dec2bin((int) b, buffer_b);

    for(i = 0; i < 64; i++)
    {
        if(buffer_a[i] == buffer_b[i])
            ;
        else
            dif++;
    }
    if(dif == 1)
    {
        for(i = 0; i < 64; i ++)
            if(buffer_a[i] != buffer_b[i])
                break;
        return i;
    }
    else
        return -1;
}
void dec2bin(int a, char *bin)
{
    int quot = a;
    int i = 1;
    
    while (quot != 0)
    {
        bin[i++] = quot % 2 - 48;
        quot = quot / 2;
    }

    return ;
}
void concheck()
{

    int start[list_size] = {0};
    int end[list_size] = {0};
    int size[list_size] = {1};
    int check,check_size;
    uint64_t check_start,check_end;
    int i;
    int j = 0;
    table_entry check_PTE;
    check_PTE.VA = PT[0].VA;
    check_PTE.PFN = PT[0].PFN;
    for (i = 1 ; i < list_size; i++)
    {
        if(check_PTE.PFN + 1 == PT[i].PFN)
        {
            size[j]++;
            if(size[j] == 2)
                start[j] = check_PTE.VA;
            end[j] = PT[i].VA;
        }
        else
            j++;
    }
    check = max(size);
    check_size = size[check];
    check_start = start[check];
    check_end = end[check];

    printf("virtual start: %x  / virtual end: %x  /  size: %d", (unsigned int)check_start, (unsigned int)check_end, check_size);
}
int max(int * size)
{
    int i,j = 0 ;
    int ret = 0;

    for(i = 0; i<list_size; i++)
    {
        if(j < size[i])
        {
            j = size[i];
            ret = i;
        }
    }
    return ret;
}
void sort(int low, int high)
{
    if(low < high)
    {
        int pi = partition (low, high);

        sort(low, pi-1);
        sort(pi+1, high);
    }
}
int partition (int low, int high)
{
    int pivot = test_pair[high].diff_bit;
    int i = (low - 1);
    int j = low;

    for (j; j <= high - 1; j++)
    {
        if (test_pair[j].diff_bit <= pivot)
        {
            i++;
            diff_swap(i, j);
        }
    }
    diff_swap(i+1, high);
    return i+1;
}
void print()
{
    int i = 0;
    for(i = 0; i < list_size; i++)
    {
        printf("%d. virtual address: %x  /  page frame number: %x\n", i, (unsigned int)PT[i].VA, (unsigned int)PT[i].PFN);
    }
    return ;
}
void diff_swap(int a, int b)
{
    pair swap_pair;
    swap_pair.first.VA = test_pair[a].first.VA;
    swap_pair.first.PFN = test_pair[a].first.PFN;
    swap_pair.second.VA = test_pair[a].second.VA;
    swap_pair.second.PFN = test_pair[a].second.PFN;
    swap_pair.diff_bit = test_pair[a].diff_bit;

    test_pair[a].first.VA = test_pair[b].first.VA;
    test_pair[a].first.PFN = test_pair[b].first.PFN;
    test_pair[a].second.VA = test_pair[b].second.VA;
    test_pair[a].second.PFN = test_pair[b].second.PFN;
    test_pair[a].diff_bit = test_pair[b].diff_bit;

    test_pair[b].first.VA = swap_pair.first.VA;
    test_pair[b].first.PFN = swap_pair.first.PFN;
    test_pair[b].second.VA = swap_pair.second.VA;
    test_pair[b].second.PFN = swap_pair.second.PFN;
    test_pair[b].diff_bit = swap_pair.diff_bit;
}

int make_table()
{
    char *first_PA, *second_PA ;
    uint64_t gap = 0;
    int i,j,k = 0;
    char test;

    int PA = 0;
    PTN = 0;
    first_PA = (char *)malloc(total_size);//virtual address TODO
    second_PA = first_PA;


    pid = getpid();
    printf("pid : %d\n",pid);
    sprintf(path_buf, "/proc/%u/pagemap", pid);
    for(i=0;i<total_size; i++)
        memset(second_PA++, '1', 1);


    for(i=0;i<total_size; i = i + page_size)
    {
       
        read_pagemap(path_buf, (unsigned long)first_PA);
        first_PA = first_PA + page_size;


    }
    printf("END\n");
    return 0;


}

inline uint64_t time_stamp()
{
    unsigned hi, lo;
    unsigned hi_, lo_;

    asm volatile (
            "cpuid;"
            "rdtsc;"
            : "=a"(lo), "=d"(hi)
            );
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
int gap_check(char *first_PA, char *second_PA)
{
    uint64_t start_f, end_f, start_s, end_s, gap_f, gap_s = 0;
    char value;
    char *address;
//    address = (char*)time_stamp();
    //Flush_Cache(first_PA, second_PA);
 
    start_f = time_stamp();
    value = *first_PA;
    end_f = time_stamp();
   
    gap_f = Check_Gap(end_f, start_f);


    printf("first operation : %p / %" PRIu64, first_PA, gap_f);
   
//    address = (char*)time_stamp();
    //Flush_Cache(first_PA, second_PA);

    start_s = time_stamp();
    value = *second_PA;
    end_s = time_stamp();
   
    gap_s = Check_Gap(end_s, start_s); 

    printf(" ,  second operation : %p / %" PRIu64, second_PA, gap_s);
    sleep(1);

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
        sleep(1);
        return -1;
    }

    //Shifting by virt-addr-offset number of bytes
    //and multiplying by the size of an address (the size of an entry in pagemap file)
    file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
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
    if(GET_BIT(read_val, 63))
    {
        PT[PTN].PFN = GET_PFN(read_val);
        PT[PTN++].VA = (unsigned long) virt_addr;
    }
        //return GET_PFN(read_val);
    else
        printf("Page not present\n");
    if(GET_BIT(read_val, 62))
        printf("Page swapped\n");
    fclose(f);
    return 0;
}
void latency_check()
{
    int i;
    char *ptr = (char *)PT[0].VA;
    for(i = 1 ; i < list_size ; i++)
    {
        gap_check(ptr, (char *)PT[i].VA);
    }
}

