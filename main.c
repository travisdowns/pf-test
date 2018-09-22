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
    
        // Test the case for page fault by making the removing the mapping between virtual and physical pages.
        //munmap(base, PAGE_SIZE*NUM_OF_PAGES);
    
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
    
        for(char *i = base; i < limit; i=i+PAGE_SIZE)
        {
    
            /*
    
            For testing the prefetch0 latency, include only _mm_prefetch. Pass either i or base to test "all miss TLB" and "all hit TLB" cases.
            For testing the prefetchw latency, include only _m_prefetchw. Pass either i or base to test "all miss TLB" and "all hit TLB" cases.
            For comparing between prefetch0 and load, use _mm_lfence.
    
            */
    
            //*((volatile char *)i);
            _mm_prefetch(i,_MM_HINT_T0);
            //_mm_lfence();

            // Supported on Broadwell and Silvermont and later.
            //_m_prefetchw(i);.
        }
    
        clock_gettime(CLOCK_MONOTONIC, &end);
        diff += 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
    
        getrusage(RUSAGE_SELF, &usage2);
    
        minorFaults += usage2.ru_minflt - usage1.ru_minflt;
        majorFaults += usage2.ru_majflt - usage1.ru_majflt;
    
        // Comment this line for the case of page fault.
        munmap(base, PAGE_SIZE*NUM_OF_PAGES);
    
    }
    
    printf("Time = %f nanoseconds\n", diff/REPEAT);
    printf("minorFaults = %f\n", minorFaults/REPEAT); // Must be very small.
    printf("majorFaults = %f\n", majorFaults/REPEAT); // Must be very small.
 
    return 0;
}