#ifndef __common_LOGGINGV2_HPP__
#define __common_LOGGINGV2_HPP__

#include <iostream>
#include <sstream>
#include <memory>
#include <cstdlib>
#include <string.h>

namespace common {

#define LOGGINGV2_NONE 0
#define LOGGINGV2_DEBUG 1
#define LOGGINGV2_INFO 2
#define LOGGINGV2_WARN 3
#define LOGGINGV2_FATAL 4

#ifdef NONE
#undef NONE
#endif
#define NONE LOGGINGV2_NONE

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG LOGGINGV2_DEBUG

#ifdef INFO
#undef INFO
#endif
#define INFO LOGGINGV2_INFO

#ifdef WARN
#undef WARN
#endif
#define WARN LOGGINGV2_WARN

#ifdef FATAL
#undef FATAL
#endif
#define FATAL LOGGINGV2_FATAL

#ifdef NDEBUG
#define LIMIT LOGGINGV2_INFO
#else
#define LIMIT LOGGINGV2_DEBUG
#endif

#define LOG(severity) COMPACT_LOG_MESSAGE(severity)
#define COMPACT_LOG_MESSAGE(s) LOG_MESSAGE_ ## s(__FILE__, __LINE__)

#define LOG_MESSAGE_0(file, lineno) common::LogMessage::New(0, file, lineno)
#define LOG_MESSAGE_1(file, lineno) common::LogMessage::New(1, file, lineno)
#define LOG_MESSAGE_2(file, lineno) common::LogMessage::New(2, file, lineno)
#define LOG_MESSAGE_3(file, lineno) common::LogMessage::New(3, file, lineno)
#define LOG_MESSAGE_4(file, lineno) common::LogMessage::New(4, file, lineno)

#define CHECK_OP(op, x, y)  (((x) op (y))? LOG(NONE) : LOG(FATAL))

#define CHECK_EQ(x, y) CHECK_OP(==, x, y)
#define CHECK_NE(x, y) CHECK_OP(!=, x, y)
#define CHECK_GT(x, y) CHECK_OP(>, x, y)
#define CHECK_GE(x, y) CHECK_OP(>=, x, y)
#define CHECK_LT(x, y) CHECK_OP(<, x, y)
#define CHECK_LE(x, y) CHECK_OP(<=, x, y)
#define CHECK(x) CHECK_EQ(x, true)

#define CHECK_NULL(x) CHECK_OP(==, x, nullptr)
#define CHECK_NOTNULL(x) CHECK_OP(!=, x, nullptr)

#define LOG_IF(severity, expr) ((expr) ? LOG(severity) : LOG(NONE))

#define LOG_EVERY_N(severity, n) LOG_EVERY_N2(severity, n, __LINE__)
#define LOG_EVERY_N2(severity, n, line) \
    static int LOG_OCCURRENCE ## line= 0;\
    static int LOG_MODULE ## line = (n); \
    LOG_OCCURRENCE ## line  ++; if (LOG_OCCURRENCE ## line == LOG_MODULE ## line) LOG_OCCURRENCE ## line -= LOG_MODULE ## line; \
    LOG_IF(severity, LOG_OCCURRENCE ## line == 1)

#define LOG_FIRST_N(severity, n) LOG_FIRST_N2(severity, n, __LINE__) 
#define LOG_FIRST_N2(severity, n, line) \
    static int LOG_OCCURRENCE ## line = 0;  \
    static int LOG_MAX ## line = (n);\
    LOG_OCCURRENCE ## line ++; \
    LOG_IF(severity, LOG_OCCURRENCE ## line <= LOG_MAX ## line)

static const char* log_literal[] = {
    "None",
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Fatal",
    nullptr
};

class LogMessage {
    public:
        LogMessage(int severity, const char* filename, const int lineno) {
            _severity = severity;
            _filename = filename;
            _lineno = lineno;
            _stream = std::shared_ptr<std::ostringstream>(new std::ostringstream());
            if (_severity != LOGGINGV2_NONE)
                (*this) << log_literal[_severity] << " [" << baseFilename(filename) << "|" << lineno << "] ";
        }

        LogMessage(const LogMessage& other) {
            _severity = other._severity;
            _filename = other._filename;
            _lineno = other._lineno;
            _stream = other._stream;
        }

        ~LogMessage() {
            if (_severity >= LIMIT) {
              std::cerr << _stream->str();
            }
            if (_severity == FATAL) {
              std::cerr << "Abort" << std::endl;
              exit(-1);
            }
        }

        static LogMessage New(int severity, const char* filename, const int lineno, bool cond = true) {
            if (cond) {
              return LogMessage(severity, filename, lineno);
            } else {
              return LogMessage(LOGGINGV2_NONE, filename, lineno);
            }
        }

        template<class T>
        LogMessage& operator<<(const T& t) {
            if (_severity >= LIMIT)
                (*_stream) << t;
            return *this;
        }

        typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
        typedef CoutType& (*StandardEndLine)(CoutType&);

        LogMessage& operator<<(StandardEndLine manip) {
            if (_severity >= LIMIT)
                manip(*_stream);
            return *this;
        }
    private:
        int _severity;
        const char* _filename;
        int _lineno;
        std::shared_ptr<std::ostringstream> _stream;

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
