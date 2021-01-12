#ifndef __common_LOGGING_HPP__
#define __common_LOGGING_HPP__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

namespace common {

#define CLASS_MAKE_LOGGER \
    private:\
            common::Logger _logger;
#define CLASS_INIT_LOGGER(classname, level)\
    _logger.setClassName(classname);\
    _logger.setLevel(level);
#define CLOG_DEBUG(...) _logger.debug( __func__, __FILE__, __LINE__, __VA_ARGS__)
#define CLOG_INFO(...) _logger.info( __func__, __FILE__, __LINE__, __VA_ARGS__)
#define CLOG_WARN(...) _logger.warn(  __func__,__FILE__, __LINE__, __VA_ARGS__)
#define CLOG_ERROR(...) _logger.error(  __func__,__FILE__, __LINE__, __VA_ARGS__)
#define CLOG_FATAL(...) _logger.fatal(  __func__,__FILE__, __LINE__, __VA_ARGS__)


#define FUNC_MAKE_LOGGER common::Logger __func_logger__;
#define FUNC_LOGGER_SET_LEVEL(level) __func_logger__.setLevel(level);
#define FLOG_DEBUG(...) __func_logger__.debug( __func__, __FILE__, __LINE__, __VA_ARGS__)
#define FLOG_INFO(...) __func_logger__.info( __func__, __FILE__, __LINE__, __VA_ARGS__)
#define FLOG_WARN(...) __func_logger__.warn(  __func__,__FILE__, __LINE__, __VA_ARGS__)
#define FLOG_ERROR(...) __func_logger__.error(  __func__,__FILE__, __LINE__, __VA_ARGS__)
#define FLOG_FATAL(...) __func_logger__.fatal(  __func__,__FILE__, __LINE__, __VA_ARGS__)

static const char* level_literal[] = {
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Fatal",
    NULL
};


enum Level {
    L_DEBUG = 0,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_FATAL
};

enum LoggerType {
    LT_CLASS,
    LT_FUNCTION
};

class Logger {
    public:
        Logger()
            : _level(L_INFO), _type(LT_FUNCTION) {}

        Logger(Level level)
            : _level(level), _type(LT_FUNCTION) {}

        void setLevel(Level level) { _level = level; }
        void setClassName(std::string cname) { _cname = cname; _type = LT_CLASS;}

        void debug(const char* funcname, const char* filename, int lineno, const char* fmt, ...) {
            va_list va;
            va_start(va, fmt);
            _log(funcname, filename, lineno, L_DEBUG, fmt, va);
            va_end(va);
        }

        void info(const char* funcname, const char* filename, int lineno, const char* fmt, ...) {
            va_list va;
            va_start(va, fmt);
            _log(funcname, filename, lineno, L_INFO, fmt, va);
            va_end(va);
        }

        void warn(const char* funcname, const char* filename, int lineno, const char* fmt, ...) {
            va_list va;
            va_start(va, fmt);
            _log(funcname, filename, lineno, L_WARN, fmt, va);
            va_end(va);
        }

        void error(const char* funcname, const char* filename, int lineno, const char* fmt, ...) {
            va_list va;
            va_start(va, fmt);
            _log(funcname, filename, lineno, L_ERROR, fmt, va);
            va_end(va);
        }

        void fatal(const char* funcname, const char* filename, int lineno, const char* fmt, ...) {
            va_list va;
            va_start(va, fmt);
            _log(funcname, filename, lineno, L_FATAL, fmt, va);
            va_end(va);
        }

    private:
        Level _level;
        std::string _cname;
        LoggerType _type;

        void _log(const char* funcname, const char* filename, int lineno, Level level, const char* fmt, va_list va) {
            if (level < _level) return;
            char fullfmt[512];
            const char* basename = baseFilename(filename);
            if (_type == LT_FUNCTION) {
                snprintf(fullfmt, 511, "%s %s @ [%s|%d] %s", level_literal[level], funcname, basename, lineno, fmt);
            } else {
                snprintf(fullfmt, 511, "%s %s:%s @ [%s|%d] %s", level_literal[level], _cname.c_str(), funcname, basename, lineno, fmt);
            }
            vfprintf(stderr, fullfmt, va);

            if (level == L_ERROR or level == L_FATAL) {
                fprintf(stderr, "Abort!\n");
                exit(-1);
            }
        }

        const char* baseFilename(const char* path) {
            const char* p = path + strlen(path);
            while (p != path) {
                if (*p == '/') {
                    return p + 1;
                }
                p --;
            }
            return path;
        }
};

}

#endif
