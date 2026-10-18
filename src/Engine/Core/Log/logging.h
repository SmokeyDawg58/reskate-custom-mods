#pragma once
#include "types.h"
#include "Engine/Core/Console/console_entry.h"
#include <Windows.h>
#include <cstdarg>
#include <filesystem>
#include <format>
#include <string>
#include <vector>
#include <utility>

namespace dingosdk::logging {
struct Options {
    std::filesystem::path directory;
    Level level{Level::info};
    bool external_console{};
    // The launcher resets the file before starting the child; the runtime appends.
    bool truncate_file{};
};
struct Status {
    std::filesystem::path directory;
    Level level{Level::info};
    bool initialized{}, file_output{}, external_console{};
};
// Initialize outside DllMain and before native hooks. File failures are nonfatal;
// the bounded in-game feed and attached debugger remain usable.
bool initialize(const Options& options) noexcept;
Options options_from_environment();
std::filesystem::path log_directory(const std::filesystem::path& game_directory);
Status status();
bool enabled(Level level) noexcept;
void set_level(Level level) noexcept;
void startup_banner() noexcept;
void flush() noexcept;
// Executables call shutdown at normal exit. The pinned runtime lives until
// process teardown; do not take logging locks or join threads from DllMain.
void shutdown() noexcept;
void write(Level level, Context context, Channel channel, std::string_view message) noexcept;
void write(Level level, Context context, Channel channel, std::wstring_view message) noexcept;
void vprintf(Level level, Context context, Channel channel, const char* format, va_list args) noexcept;
void printf(Level level, Context context, Channel channel, const char* format, ...) noexcept;
void write(Level level, Channel channel, std::string_view message) noexcept;
void write(Level level, Channel channel, std::wstring_view message) noexcept;
void vprintf(Level level, Channel channel, const char* format, va_list args) noexcept;
void printf(Level level, Channel channel, const char* format, ...) noexcept;
template<class... Args>
void log(Level level, Context context, Channel channel, std::format_string<Args...> format, Args&&... args) noexcept {
#if defined(DINGOSDK_LEAN_PLAYTEST) && !defined(DINGOSDK_LEAN_LOGGING)
    (void)level; (void)context; (void)channel; (void)format;
    ((void)args, ...);
#else
    const auto saved = GetLastError();
    try {
        if (enabled(level) || (channel == Channel::command && level != Level::off)) write(level, context, channel, std::format(format, std::forward<Args>(args)...));
    } catch (...) { write(Level::error, Channel::runtime, "Could not format a log message."); }
    SetLastError(saved);
#endif
}
template<class... Args>
void log(Level level, Channel channel, std::format_string<Args...> format, Args&&... args) noexcept {
    log(level, default_context(channel), channel, format, std::forward<Args>(args)...);
}
// Structured native observations retain their fields. Human-readable output goes
// through the same sinks as ordinary messages; trace level includes raw payloads.
void event(Channel channel, std::string_view json, Level level = Level::debug) noexcept;
void event(Context context, Channel channel, std::string_view json, Level level = Level::debug) noexcept;
std::vector<ConsoleLogLine> snapshot_after(std::uint64_t sequence);
std::uint64_t latest_sequence() noexcept;
}
