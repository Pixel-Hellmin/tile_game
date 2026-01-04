#if !defined(PROFILER_CPP)
//TODO(Fermin): Average cycle/ms time for each function/block.
//additionally to printing total counts, cycles, and % we could
//also print average cycles for each block/function.

#include "platform_metrics.cpp"

#ifndef PROFILER
#define PROFILER 0
#endif

#ifndef READ_BLOCK_TIMER
#define READ_BLOCK_TIMER read_CPU_timer
#endif

#if PROFILER

struct profile_anchor
{
    u64 TSC_elapsed_exclusive; // NOTE(casey): Does NOT include children
    u64 TSC_elapsed_inclusive; // NOTE(casey): DOES include children
    u64 hit_count;
    u64 processed_byte_count;
    char const *label;
};
global_variable profile_anchor global_profiler_anchors[4096];
global_variable u32 global_profiler_parent;

struct profile_block
{
    profile_block(char const *label_, u32 anchor_index_, u64 byte_count)
    {
        parent_index = global_profiler_parent;

        anchor_index = anchor_index_;
        label = label_;

        profile_anchor *anchor = global_profiler_anchors + anchor_index;
        old_TSC_elapsed_inclusive = anchor->TSC_elapsed_inclusive;
        anchor->processed_byte_count += byte_count;

        global_profiler_parent = anchor_index;
        start_TSC = READ_BLOCK_TIMER();
    }

    ~profile_block(void)
    {
        u64 elapsed = READ_BLOCK_TIMER() - start_TSC;
        global_profiler_parent = parent_index;

        profile_anchor *parent = global_profiler_anchors + parent_index;
        profile_anchor *anchor = global_profiler_anchors + anchor_index;

        parent->TSC_elapsed_exclusive -= elapsed;
        anchor->TSC_elapsed_exclusive += elapsed;
        anchor->TSC_elapsed_inclusive = old_TSC_elapsed_inclusive + elapsed;
        ++anchor->hit_count;
       
        anchor->label = label;
    }

    char const *label;
    u64 old_TSC_elapsed_inclusive;
    u64 start_TSC;
    u32 anchor_index;
    u32 parent_index;
};

//NOTE(Fermin): name_concat and name_concat2 are "workarounds for the fact
//that C++’s macro preprocessor is terrible, and can’t merge the value
//of a preprocessor macro with an identifier any other way" -Casey
#define name_concat2(a, b) a##b
#define name_concat(a, b) name_concat2(a, b)
#define time_bandwidth(name, byte_count) profile_block name_concat(block, __LINE__)(name, __COUNTER__ + 1, byte_count)
#define profiler_end_of_compilation_unit static_assert(__COUNTER__ < array_count(global_profiler_anchors), "Number of profile points exceeds size of profiler::anchors array");

static void print_time_elapsed(u64 total_TSC_elapsed, u64 timer_freq, profile_anchor *anchor)
{
    f64 percent = 100.0 * ((f64)anchor->TSC_elapsed_exclusive / (f64)total_TSC_elapsed);
    printf("  %s[%llu]: %llu (%.2f%%", anchor->label, anchor->hit_count, anchor->TSC_elapsed_exclusive, percent);
    if (anchor->TSC_elapsed_inclusive != anchor->TSC_elapsed_exclusive)
    {
        f64 percent_with_children = 100.0 * ((f64)anchor->TSC_elapsed_inclusive / (f64)total_TSC_elapsed);
        printf(", %.2f%% w/children", percent_with_children);
    }
    printf(")");

    if (anchor->processed_byte_count)
    {
        f64 megabyte = 1024.0f*1024.0f;
        f64 gigabyte = megabyte*1024.0f;

        f64 seconds = (f64)anchor->TSC_elapsed_inclusive / (f64)timer_freq;
        f64 bytes_per_second = (f64)anchor->processed_byte_count / seconds;
        f64 megabytes = (f64)anchor->processed_byte_count / (f64)megabyte;
        f64 gigabytes_per_second = bytes_per_second / gigabyte;

        printf(" %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
    }

    printf("\n");
}

static void print_anchor_data(u64 total_CPU_elapsed, u64 timer_freq)
{
    for (u32 anchor_index = 0; anchor_index < array_count(global_profiler_anchors); ++anchor_index)
    {
        profile_anchor *anchor = global_profiler_anchors + anchor_index;
        if (anchor->TSC_elapsed_inclusive)
        {
            print_time_elapsed(total_CPU_elapsed, timer_freq, anchor);
        }
    }
}

#else

#define time_bandwidth(...)
#define print_anchor_data(...)
#define profiler_end_of_compilation_unit

#endif

struct profiler
{
    u64 start_TSC;
    u64 end_TSC;
};
global_variable profiler global_profiler;

#define time_block(name) time_bandwidth(name, 0);
#define time_function time_block(__func__)

inline u64 estimate_block_timer_frequency()
{
    u64 os_freq = get_OS_timer_freq();

    u64 cpu_start = READ_BLOCK_TIMER();
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
    u64 cpu_end = READ_BLOCK_TIMER();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 result = 0;
    if (os_elapsed)
    {
        result = os_freq * cpu_elapsed / os_elapsed; 
    }
    
    return result;
}

static void begin_profile(void)
{
    global_profiler.start_TSC = READ_BLOCK_TIMER();
}

static void end_and_print_profile()
{
    global_profiler.end_TSC = READ_BLOCK_TIMER();
    u64 timer_freq = estimate_block_timer_frequency();

    printf("\n------------- Performance Metrics V2 -------------\n");
    
    u64 total_CPU_elapsed = global_profiler.end_TSC - global_profiler.start_TSC;

    if (timer_freq)
    {
        printf("Total time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)total_CPU_elapsed / (f64)timer_freq, timer_freq);
    }

    print_anchor_data(total_CPU_elapsed, timer_freq);
}

#define PROFILER_CPP
#endif
