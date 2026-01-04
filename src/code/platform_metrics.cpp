#if _WIN32

#include <intrin.h>
#include <windows.h>

static u64 get_OS_timer_freq(void)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

static u64 read_OS_timer(void)
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

#else

#include <x86intrin.h>
#include <sys/time.h>

static u64 get_OS_timer_freq(void)
{
	return 1000000;
}

static u64 read_OS_timer(void)
{
	// NOTE(casey): The "struct" keyword is not necessary here when compiling in C++,
	// but just in case anyone is using this file from C, I include it.
	struct timeval value;
	gettimeofday(&value, 0);
	
	u64 result = GetOSTimerFreq()*(u64)value.tv_sec + (u64)value.tv_usec;
	return result;
}

#endif

/* NOTE(casey): This does not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about 
   static functions that aren't used. So "inline" is just the simplest way 
   to tell them to stop complaining about that. */
inline u64 read_CPU_timer(void)
{
	// NOTE(casey): If you were on ARM, you would need to replace __rdtsc
	// with one of their performance counter read instructions, depending
	// on which ones are available on your platform.
	
	return __rdtsc();
}

inline u64 estimate_cpu_frequency()
{
    u64 os_freq = get_OS_timer_freq();
    u64 cpu_start = read_CPU_timer();
    u64 os_start = read_OS_timer();
    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 ms_wait_time = 100;
    u64 os_wait_time = os_freq * ms_wait_time / 1000;
    while (os_elapsed < os_wait_time)
    {
        os_end = read_OS_timer();
        os_elapsed = os_end - os_start;
    }
    u64 cpu_end = read_CPU_timer();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 result = 0;
    if (os_elapsed)
    {
        result = os_freq * cpu_elapsed / os_elapsed; 
    }
    
    return result;
}

inline void print_cpu_frequency(u64 wait_time)
{
    printf("\n------------- Estimated CPU Frequency -------------\n");
    u64 os_freq = get_OS_timer_freq();
    printf("   OS Freq: %llu\n", os_freq);

    u64 cpu_start = read_CPU_timer();
    u64 os_start = read_OS_timer();
    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 os_wait_time = os_freq * wait_time / 1000;
    while (os_elapsed < os_wait_time)
    {
        os_end = read_OS_timer();
        os_elapsed = os_end - os_start;
    }
    u64 cpu_end = read_CPU_timer();
    u64 cpu_elapsed = cpu_end - cpu_start;
    u64 cpu_freq = 0;
    if (os_elapsed)
    {
      //cpu_tick/sec  = os_tick/sec * cpu_tick    / os_tick
        cpu_freq      = os_freq     * cpu_elapsed / os_elapsed; 
    }

    printf("  OS Timer: %llu -> %llu = %llu elapsed\n", os_start, os_end, os_elapsed);
	printf("OS Seconds: %.4f\n", (f64)os_elapsed/(f64)os_freq);
    printf(" CPU Timer: %llu -> %llu = %llu elapsed\n", cpu_start, cpu_end, cpu_elapsed);
    printf("  CPU Freq: %llu (guessed)\n", cpu_freq);
}
