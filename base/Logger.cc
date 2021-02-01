
/******************************************************
*   Copyright (C)2019 All rights reserved.
*
*   Author        : owb
*   Email         : 2478644416@qq.com
*   File Name     : Logger.cc
*   Last Modified : 2019-11-23 14:14
*   Describe      :
*
*******************************************************/

#include "ideal/base/Logger.h"
#include "ideal/base/CurrentThread.h"
#include "ideal/base/Timestamp.h"
#include "ideal/base/TimeZone.h"

#include <errno.h>

namespace ideal {

__thread time_t t_lastSecond;  // 缓存上一秒，避免频繁生成tm
__thread char t_time[64];      // 格式化日期时间
__thread char t_errnobuf[512]; // errno错误信息

const char* strerror_tl(int savedErrno) {
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

}


using namespace ideal;

const char* LogLevelName[Logger::LOG_LEVEL_NUM] = {
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL "
};

Logger::LogLevel initLogLevel() {
    if(::getenv("IDEAL_LOG_TRACE"))
        return Logger::TRACE;
    else if(::getenv("IDEAL_LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}


void defaultOutput(const char* msg, int len) {
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
}

void defaultFlush(){
    fflush(stdout);
}

TimeZone g_logTimeZone;
Logger::LogLevel g_logLevel = initLogLevel();
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;


Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line) :
    _ts(Timestamp::now()),
    _stream(),
    _level(level),
    _line(line),
    _basename(file) {
    formatTime();   // 时间ts
    CurrentThread::tid();   // 线程id
    _stream << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
    _stream << T(LogLevelName[level], 6);   // 日志level
    if(savedErrno != 0) {  // 错误信息errno
        _stream << strerror_tl(savedErrno) << " (errno = " << savedErrno << ") ";
    }

    _stream << _basename << ':' << _line << " - "; // 文件名 行
}

void Logger::Impl::formatTime() {
    int64_t microsecondsSinceEpoch = _ts.microsecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microsecondsSinceEpoch / Timestamp::kMicrosecondsPerSecond);
    int microseconds = static_cast<int>(microsecondsSinceEpoch % Timestamp::kMicrosecondsPerSecond);
    if(seconds != t_lastSecond) {
        t_lastSecond = seconds;
        struct tm tm_time;
        if(g_logTimeZone.valid())
            tm_time = g_logTimeZone.toLocalTime(seconds);
        else
            ::gmtime_r(&seconds, &tm_time);
  
        // 格式化时间
        size_t len = ::strftime(t_time, sizeof(t_time), "%F %H:%M:%S", &tm_time);
        assert(len == 19);
        (void)len;
    }

    if(g_logTimeZone.valid()) {
        Fmt us(".%06d ", microseconds);
        assert(us.length() == 8);
        _stream << T(t_time, 19) << T(us.data(), 8);
    }
    else {
        Fmt us(".%06dZ ", microseconds);  // 国际标准时间
        assert(us.length() == 9);
        _stream << T(t_time, 19) << T(us.data(), 9);
    }
}

void Logger::Impl::finish() {
    _stream << '\n';
}

Logger::Logger(SourceFile file, int line) :
    _impl(INFO, 0, file, line) {
}

Logger::Logger(SourceFile file, int line, LogLevel level) :
    _impl(level, 0, file, line) {
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func) :
    _impl(level, 0, file, line) {
    _impl._stream << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort) :
    _impl(toAbort ? FATAL:ERROR, errno, file, line) {
}

Logger::~Logger() {
    _impl.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if(_impl._level == FATAL) {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz) {
    g_logTimeZone = tz;
}

Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}
