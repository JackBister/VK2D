#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <type_traits>

#include "LogAppender.h"
#include "Util/DllExport.h"

template <typename T>
concept Enum = std::is_enum<T>::value;

template <Enum T, typename CharT>
struct std::formatter<T, CharT> : std::formatter<std::underlying_type_t<T>, CharT> {
    template <class FormatContext>
    auto format(T t, FormatContext & ctx)
    {
        auto cast = static_cast<typename std::underlying_type<T>::type>(t);
        return std::formatter<std::underlying_type<T>::type, CharT>::format(cast, ctx);
    }
};

template <typename CharT>
struct std::formatter<std::filesystem::path, CharT> : std::formatter<std::string, CharT> {
    template <class FormatContext>
    auto format(std::filesystem::path p, FormatContext & ctx)
    {
        return std::formatter<std::string, CharT>::format(p.string(), ctx);
    }
};

template <class T, typename CharT>
struct std::formatter<std::optional<T>, CharT> : std::formatter<T, CharT> {
    template <class FormatContext>
    auto format(std::optional<T> opt, FormatContext & ctx)
    {
        if (opt.has_value()) {
            return std::formatter<T, CharT>::format(opt.value(), ctx);
        } else {
            return std::formatter<std::string, CharT>::format("optional::empty", ctx);
        }
    }
};

template <class T, typename CharT>
struct std::formatter<T *, CharT> : std::formatter<void *, CharT> {
    template <class FormatContext>
    auto format(T * opt, FormatContext & ctx)
    {
        return std::formatter<void *, CharT>::format((void *)opt, ctx);
    }
};

class EAPI Logger
{
public:
    Logger(std::string name, std::shared_ptr<LogAppender> appender);

    static Logger Create(std::string const & name);

    template <class... Args>
    void Trace(std::string_view fmt, Args const &... args) const
    {
        std::string messageString = std::format(fmt, args...);
        appender->Append(LogMessage(name, LogLevel::TRACE, messageString));
    }

    template <class... Args>
    void Info(std::string_view fmt, Args const &... args) const
    {
        std::string messageString = std::format(fmt, args...);
        appender->Append(LogMessage(name, LogLevel::INFO, messageString));
    }

    template <class... Args>
    void Warn(std::string_view fmt, Args const &... args) const
    {
        std::string messageString = std::format(fmt, args...);
        appender->Append(LogMessage(name, LogLevel::WARN, messageString));
    }

    template <class... Args>
    void Error(std::string_view fmt, Args const &... args) const
    {
        std::string messageString = std::format(fmt, args...);
        appender->Append(LogMessage(name, LogLevel::ERROR, messageString));
    }

    template <class... Args>
    void Severe(std::string_view fmt, Args const &... args) const
    {
        std::string messageString = std::format(fmt, args...);
        appender->Append(LogMessage(name, LogLevel::SEVERE, messageString));
    }

private:
    std::shared_ptr<LogAppender> appender;
    std::string name;
};
