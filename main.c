//gcc -O3 main.c

#include <sys/mman.h> // mmap
#include <stdio.h> // printf
#include <x86intrin.h> // _m_prefetchw & _mm_prefetch
#include <time.h> // clock_gettime
#include <sys/time.h> // getrusage
#include <sys/resource.h> // getrusage

// Because I'm using clock_gettime, the measurments are not reliable with NUM_OF_PAGES much less 10 thousands.
#define NUM_OF_PAGES (size_t)100000

// The number of times to repeat the experiment to make more reliable measurments.
#define REPEAT 10

// My code does not support other page sizes. Although it'd be interesting to experiment with different sizes.
#define PAGE_SIZE 4096

// Exactly one of these must be defined.
//#define ALL_HIT_TLB
#ifdef ALL_HIT_TLB
#define TEST_CASE 0
#elif defined(ALL_MISS_TLB)
#define TEST_CASE 1
#elif defined(FAULT_DIFFERENT_PAGE)
#define TEST_CASE 2
#elif defined(FAULT_SAME_PAGE)
#define TEST_CASE 3
#else
#error "No test case defined"
#endif

// In addition, you can define zero or one of these two:
//#define LOAD_LFENCE
//#define PREFETCH_LFENCE

// Exactly one of these must be defined.
#define PREFETCHh
//#define PREFETCW

// Prefetch hint for the PREFETCHh case.
// Ignored for the PREFETCW case.
#define PREFETCH_HINT _MM_HINT_T0

int main()
{
    
    double diff = 0;
    double minorFaults = 0;
    double majorFaults = 0;
    
    for(int r=0;r<REPEAT;++r)
    {
        char *base = (char *) mmap(0, PAGE_SIZE*NUM_OF_PAGES, PROT_READ|PROT_WRITE, MAP_PRIVATE| MAP_ANONYMOUS, 0, 0);
        char *limit = base + PAGE_SIZE*NUM_OF_PAGES;
    
        for(char *i = base; i < limit; i=i+PAGE_SIZE)
        {
            // Force the OS to allocate a unique private physical page.
            *i = 1;
        }
    
        struct rusage usage1, usage2;
        getrusage(RUSAGE_SELF, &usage1);

        #if TEST_CASE == 2 || TEST_CASE == 3
            munmap(base, PAGE_SIZE*NUM_OF_PAGES);
        #endif
    
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
    
        for(char *i = base; i < limit; i=i+PAGE_SIZE)
        {

            #ifdef LOAD_LFENCE
                *((volatile char *)i);
                _mm_lfence();
            #elif defined(PREFETCH_LFENCE)
                #define FENCE _mm_lfence();
            #else
                #define FENCE
            #endif

            #if !defined(LOAD_LFENCE)
                #if TEST_CASE == 0 || TEST_CASE == 3
                    #ifdef PREFETCHh
                        _mm_prefetch(base, PREFETCH_HINT);
                    #elif defined(PREFETCW)
                        _m_prefetchw(base);
                    #endif
                #elif TEST_CASE == 1 || TEST_CASE == 2
                    #ifdef PREFETCHh
                        _mm_prefetch(i, PREFETCH_HINT);
                    #elif defined(PREFETCW)
                        _m_prefetchw(i);
                    #endif
                #endif
                FENCE
            #endif
        }
    
        clock_gettime(CLOCK_MONOTONIC, &end);
        diff += 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
    
        getrusage(RUSAGE_SELF, &usage2);
    
        minorFaults += usage2.ru_minflt - usage1.ru_minflt;
        majorFaults += usage2.ru_majflt - usage1.ru_majflt;
    
        #if TEST_CASE != 2 && TEST_CASE != 3
            munmap(base, PAGE_SIZE*NUM_OF_PAGES);
        #endif
    
    }
    
    printf("Time        = %5.0f ns\n", diff/REPEAT);
    printf("Time/access = %5.2f ns\n", diff/REPEAT/NUM_OF_PAGES);
    printf("minorFaults = %f\n", minorFaults/REPEAT); // Must be very small.
    printf("majorFaults = %f\n", majorFaults/REPEAT); // Must be very small.
 
    return 0;
}