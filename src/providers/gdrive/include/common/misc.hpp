#ifndef __common_MISC_HPP__
#define __common_MISC_HPP__

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <ctime>
#else
#include <unistd.h>
#include <sys/time.h>
#include <ctime>
#endif

#include <map>

namespace common {

typedef std::map<std::string, std::string> string_map;

#define CLASS_NOCOPY(class) \
    private: \
        class& operator=(class& other); \
        class(const class& other);

static inline size_t get_ncpu() {
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;
 
    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
 
    if(count < 1) {
    nm[1] = HW_NCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
    if(count < 1) { count = 1; }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

static int64_t get_time_in_micro()
{
#ifdef WIN32
     /* Windows */
    FILETIME ft;
    LARGE_INTEGER li;

    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
     * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    int64_t ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 1000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

    return ret;
#else
    /* Linux */
    struct timeval tval;

    gettimeofday(&tval, NULL);

    int64_t ret = tval.tv_usec;
    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (tval.tv_sec * 1000000);
    return ret;
#endif
} 
}

#endif
