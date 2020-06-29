#include <chrono>
#include <fstream>
#include <queue>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "config.hpp"
#include "logging.hpp"

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#include "StackWalker.h"
#else
#include <os/log.h>
#endif

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace fdn {

std::mutex g_codeLocationsMutex;
std::vector<const CodeLocation*> g_codeLocations;

size_t register_CodeLocation(const CodeLocation* location)
{
    std::lock_guard<std::mutex> guard(g_codeLocationsMutex);
    g_codeLocations.push_back(location);
    return g_codeLocations.size() - 1;
}

CodeLocation::CodeLocation(const std::string& file_, size_t line_)
    : file(file_), line(line_)
{
    handle = register_CodeLocation(this);
};

enum class LoggerLocationStyle : int {
    Off = 0,
    ShortWithThreadName = 1,
    LongWithThreadName = 2
};

NLOHMANN_JSON_SERIALIZE_ENUM(LoggerLocationStyle, {
    {LoggerLocationStyle::ShortWithThreadName, "shortWithThreadName"},  // first is default
    {LoggerLocationStyle::Off, "off"},
    {LoggerLocationStyle::LongWithThreadName, "longWithThreadName"}
    });


NLOHMANN_JSON_SERIALIZE_ENUM(LogMessageSeverity, {
    {LogMessageSeverity::Info, "info"},  // first is default
    {LogMessageSeverity::Off, "off"},
    {LogMessageSeverity::Debug, "debug"},
    {LogMessageSeverity::Warning, "warning"},
    {LogMessageSeverity::Error, "completed"},
    {LogMessageSeverity::Fatal, "fatal"}
});

class Logger
{
public:
    Logger(LogMessageSeverity threshold, PathType logPath, LoggerLocationStyle style)
        : threshold_(threshold),
        worker_(&Logger::messageOutputLoop, this, logPath, style)
    {
        nameThread("main"s);
    }
    ~Logger()
    {
        close();
    }

    struct LogMessage {
        const CodeLocation* codeLocation;
        LogMessageSeverity severity;  // severity==NAME_THREAD means use "text" as name of threadId
        std::string text;
        std::thread::id threadId;
    };

    void close() {
        if (!closed_) {
            quit_ = true;
            worker_.join();
            closed_ = true;
        }
    }

    void pushMessage(LogMessage&& logMessage)
    {
        LogMessage lm = std::move(logMessage);
        auto& [_, severity, __, ___] = lm;

        if (severity < threshold_ || closed_)
            return;
        bool wasFatal = (severity == LogMessageSeverity::Fatal);

        {
            std::lock_guard<std::mutex> guard(messagesMutex_);
            messages_.push(std::move(lm));
        }

        if (wasFatal)
            close();  // we make sure all output is complete, as we're in hard crash and may lose the opportunity after
    }

    void nameThread(const std::string& name)
    {
        pushMessage(
            LogMessage{
                nullptr,
                LogMessageSeverity::NAME_THREAD,  // hack which triggers adding a name
                name,
                std::this_thread::get_id()
            }
        );
    }

    static Logger& theLogger();

private:
    std::map<std::thread::id, std::string> threadNames_;

    void handleMessage(const LogMessage& message, std::ofstream &out, LoggerLocationStyle locationStyle)
    {
        const auto& [codeLocation, severity, text, threadId] = message;

        if (severity == LogMessageSeverity::NAME_THREAD) {  // hook to set thread names
            threadNames_.insert({ threadId, "["s + text + "]"s });
        }
        else
        {
            auto threadNameIt = threadNames_.find(threadId);
            if (threadNameIt == threadNames_.end()) {
                std::stringstream fakeName;
                fakeName << "[" << threadId << "]";
                threadNameIt = threadNames_.insert({ threadId, fakeName.str() }).first;
            }

            std::stringstream full;

            const char* severitiesAsString[] = { "", "DEBUG: ", "INFO: ", "WARNING: ", "ERROR: ", "FATAL: " };
            auto severityAsString = severitiesAsString[int(severity)];
            switch (locationStyle) {
            case LoggerLocationStyle::Off:
                full << severityAsString << text;
                break;
            case LoggerLocationStyle::ShortWithThreadName:
                full
                    << fs::path(codeLocation->file).filename().string()
                    << "(" << codeLocation->line << ") "
                    << threadNameIt->second << " " << severityAsString << text;
                break;
            case LoggerLocationStyle::LongWithThreadName:
                full << codeLocation->file << "(" << codeLocation->line << ") "
                    << threadNameIt->second << " " << severityAsString << text;
                break;
            }
            out << full.str();
            out.flush();
#ifdef WIN32
            OutputDebugString(full.str().c_str());
#else
            os_log_debug(OS_LOG_DEFAULT, "%s - %s", FOUNDATION_CODEC_NAME, full.str().c_str());
#endif
        }
    }

    void messageOutputLoop(PathType logPath, LoggerLocationStyle locationStyle)
    {
        PathType logFileName =
            logPath / (FOUNDATION_CODEC_NAME "-log.txt");
        std::ofstream out;
        if (logFileName != "")
            out.open(logFileName.c_str());

        while (!quit_)
        {
            do {
                // try to dequeue a message
                std::optional<LogMessage> logMessage;
                std::unique_lock<std::mutex> lock(messagesMutex_, std::try_to_lock);
                if (lock.owns_lock())
                {
                    if (!messages_.empty())
                    {
                        logMessage = messages_.front();
                        messages_.pop();
                    }
                }

                // if a message was dequeued, used it
                if (logMessage)
                {
                    handleMessage(*logMessage, out, locationStyle);
                }
                else {
                    break;
                }
            } while (true); // continue handling while we weren't blocked and got a message from the queue

            std::this_thread::sleep_for(2ms);
        }

        // after quit_ flush queue as far as possible
        {
            // this time wait for the mutex
            std::lock_guard<std::mutex> guard(messagesMutex_);
            while (!messages_.empty())
            {
                auto logMessage = messages_.front();
                handleMessage(logMessage, out, locationStyle);
                messages_.pop();
            }
        }

        out.close();
    }
  
    LogMessageSeverity threshold_;
    bool quit_{ false };
    bool closed_{ false };
    std::thread worker_;

    mutable std::mutex messagesMutex_;
    mutable std::queue<LogMessage> messages_;

    std::thread outputThread;
};

struct LogConfiguration
{
    LogMessageSeverity threshold{ LogMessageSeverity::Info }; // only severity >= threshold are logged
    PathType path{ "" };               // "" means no log
    LoggerLocationStyle style{ LoggerLocationStyle::ShortWithThreadName };
};

void from_json(const json& j, LogConfiguration& lc) {
    try {
        j.at("threshold").get_to(lc.threshold);
    }
    catch (...)
    {
    }
    try {
        std::string s;
        j.at("path").get_to(s);
        lc.path = s;
    }
    catch (...)
    {
    }
    try {
        j.at("style").get_to(lc.style);
    }
    catch (...)
    {
    }
}

#ifdef WIN32
class FDNStackWalker : public StackWalker
{
public:
    FDNStackWalker() : StackWalker() {}

    std::stringstream ss;

protected:
    virtual void OnOutput(LPCSTR szText) {
        ss << szText;
        StackWalker::OnOutput(szText);
    }
};

// The exception filter function:
extern "C" {
    LONG WINAPI FDNExpFilter(EXCEPTION_POINTERS* pExp, DWORD dwExpCode)
    {
        FDNStackWalker sw;
        sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
        FDN_FATAL("SEH exception thrown - ", sw.ss.str());
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

#endif

class StdTerminateCrashHandlerInstaller
{
public:
    StdTerminateCrashHandlerInstaller()
    {
        s_previousHandler_ = std::get_terminate();

        std::set_terminate([]() {
            FDN_FATAL("std::terminate called"); // !!! should try to set up internals prior to crash so no alloc
#ifdef WIN32
            FDNStackWalker stackwalker;
            stackwalker.ShowCallstack();
#endif
            s_previousHandler_();
        });
    }

    static StdTerminateCrashHandlerInstaller& theStdCrashHandlerInstaller();

private:
    static std::terminate_handler s_previousHandler_;
};
std::terminate_handler StdTerminateCrashHandlerInstaller::s_previousHandler_;

StdTerminateCrashHandlerInstaller& StdTerminateCrashHandlerInstaller::theStdCrashHandlerInstaller()
{
    static StdTerminateCrashHandlerInstaller theStdCrashHandlerInstaller;
    return theStdCrashHandlerInstaller;
}

Logger& Logger::theLogger()
{
    static LogConfiguration configuration = [&](...) {
        try {
            return fdn::config().at("logging").get<LogConfiguration>();
        }
        catch (...)
        {
        }
        return LogConfiguration();
    }();
    static Logger logger(configuration.threshold, configuration.path, configuration.style);
    StdTerminateCrashHandlerInstaller::theStdCrashHandlerInstaller(); // !!! would be better to have this called at initial API hooks
    return logger;
}

// external entrypoint
void logMessage(const CodeLocation& location, LogMessageSeverity severity, const std::string& msg)
{
    Logger::theLogger().pushMessage(
        fdn::Logger::LogMessage{
            &location,
            severity,
            msg,
            std::this_thread::get_id()
        }
    );
}

void nameThread(const std::string& name)
{
    fdn::Logger::theLogger().nameThread(name);
}

}  // namespace fdn

