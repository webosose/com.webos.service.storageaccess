#ifndef __common_TIMER_HPP__
#define __common_TIMER_HPP__

#include <string.h>

#include "common/misc.hpp"

namespace common {


class Timer {
    CLASS_NOCOPY(Timer)
    public:
        Timer() {
            _start = get_time_in_micro();
        }

        void start() {
            _start = get_time_in_micro();
        }

        int64_t elapsed() {
            _end = get_time_in_micro();
            return _end - _start;
        }


    private:
        int64_t _start;
        int64_t _end;
};

class ScopeTimer {
    public:
        ScopeTimer(const char* name)
            :_timer(), _name(name) 
        {
        }

        ~ScopeTimer() {
            int64_t elapsed = _timer.elapsed();
            fprintf(stderr, "[%s]: %fs\n", _name.c_str(), elapsed * 1.0 / 1000000);
        }

    private:
        Timer _timer;
        std::string  _name;
};

}

#endif
