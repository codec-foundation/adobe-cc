#pragma once

#include <sstream>
#include <string>

// bare-bones logger
//   configured from global configuration setting "logging"
//     {
//       ...
//       "logging": {
//          "path": "/path/to/where/the/logfile/should/go",
//          "threshold": "info"
//       }
//     }
//
//     message severities are
//        "off", "debug", "info", "warning", "error", "fatal"
//     all messages at or above threshold are logged
//     default threshold is "info"
//     will try to to output to a debugger, if attached
//     will try to write to a file in the location specified in the configuration
//     no path = no logfile
//
//   with no configuration
//     will try to to output to a debugger, if attached
//
//   all messages go via a queue which is checked every 2ms for output
//   this allows usage in worker threads, but means that messages might not be exactly synchronized with
//   other systems
//
//   use as
//     FDN_<log level>(things, to, output, ...);
//   eg
//     FDN_INFO("we are now doing this", b, "something", 17.0);
//     FDN_INFO("we are now doing that", d);
//
//     FDN_DEBUG("testvar7:", testvar7);
//     FDN_WARN("disk space seems low");
//     FDN_ERROR("ran out of disk space");
//     FDN_FATAL("heap corruption detected; exiting");
//
//   other functions
//     FDN_NAME_THREAD("worker_thread_7");    // output this name with any messages from the current thread

namespace fdn
{
    enum class LogMessageSeverity : int {
        Off = 0,      // invalid / off
        Debug = 1,    // for debugging only; usually turned off
        Info = 2,     // informational
        Warning = 3,  // potential cause of later error noted
        Error = 4,    // hit bad data or operating condition, may have to stop early 
        Fatal = 5,    // in the process of crashing, eg seg fault

        NAME_THREAD = 99   // for implementation, do not use
    };
    struct CodeLocation {
        CodeLocation(const std::string& file, size_t line);

        std::string file;
        size_t line;

        size_t handle;  // registered into a table on construction
    };
    void logMessage(const CodeLocation& codeLocation, LogMessageSeverity severity, const std::string& msg);
    void nameThread(const std::string& name);
    void fatalError(const std::string& msg);   // null ptr read / write, segfault, div by 0, crash imminent
}

#define FDN_DEBUG(...) { static fdn::CodeLocation __FDN_CODELOCATION(__FILE__, __LINE__); FdnDebugImplTmp(__FDN_CODELOCATION, __VA_ARGS__); }
template <typename ...Args>
void FdnDebugImplTmp(const fdn::CodeLocation& codeLocation, Args&& ...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args)) << '\n';
    fdn::logMessage(codeLocation, fdn::LogMessageSeverity::Debug, stream.str());
}

#define FDN_INFO(...) { static fdn::CodeLocation __FDN_CODELOCATION(__FILE__, __LINE__); FdnInfoImplTmp(__FDN_CODELOCATION, __VA_ARGS__); }
template <typename ...Args>
void FdnInfoImplTmp(const fdn::CodeLocation& codeLocation, Args&& ...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args)) << '\n';
    fdn::logMessage(codeLocation, fdn::LogMessageSeverity::Info, stream.str());
}

#define FDN_WARNING(...) { static fdn::CodeLocation __FDN_CODELOCATION(__FILE__, __LINE__); FdnWarningImplTmp(__FDN_CODELOCATION, __VA_ARGS__); }
template <typename ...Args>
void FdnWarningImplTmp(const fdn::CodeLocation& codeLocation, Args&& ...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args)) << '\n';
    fdn::logMessage(codeLocation, fdn::LogMessageSeverity::Warning, stream.str());
}

#define FDN_ERROR(...) { static fdn::CodeLocation __FDN_CODELOCATION(__FILE__, __LINE__); FdnErrorImplTmp(__FDN_CODELOCATION, __VA_ARGS__); }
template <typename ...Args>
void FdnErrorImplTmp(const fdn::CodeLocation& codeLocation, Args&& ...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args)) << '\n';
    fdn::logMessage(codeLocation, fdn::LogMessageSeverity::Error, stream.str());
}

#define FDN_FATAL(...) { static fdn::CodeLocation __FDN_CODELOCATION(__FILE__, __LINE__); FdnFatalImplTmp(__FDN_CODELOCATION, __VA_ARGS__); }
template <typename ...Args>
void FdnFatalImplTmp(const fdn::CodeLocation& codeLocation, Args&& ...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args)) << '\n';
    fdn::logMessage(codeLocation, fdn::LogMessageSeverity::Fatal, stream.str());
}

#define FDN_NAME_THREAD(name) { fdn::nameThread(name); }
