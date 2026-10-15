#include "logging.h"

// A shipping build has no sinks, files, native event recorder or crash helper.
namespace dingosdk::logging {
bool initialize(const Options&) noexcept { return true; }
Options options_from_environment() { Options options; options.level=Level::off; return options; }
std::filesystem::path log_directory(const std::filesystem::path& game) { return game / L"logs"; }
Status status() { Status value; value.level=Level::off; value.initialized=true; return value; }
bool enabled(Level) noexcept { return false; }
void set_level(Level) noexcept {}
void startup_banner() noexcept {}
void flush() noexcept {}
void shutdown() noexcept {}
void write(Level, Context, Channel, std::string_view) noexcept {}
void write(Level, Context, Channel, std::wstring_view) noexcept {}
void vprintf(Level, Context, Channel, const char*, va_list) noexcept {}
void printf(Level, Context, Channel, const char*, ...) noexcept {}
void write(Level, Channel, std::string_view) noexcept {}
void write(Level, Channel, std::wstring_view) noexcept {}
void vprintf(Level, Channel, const char*, va_list) noexcept {}
void printf(Level, Channel, const char*, ...) noexcept {}
void event(Channel, std::string_view, Level) noexcept {}
void event(Context, Channel, std::string_view, Level) noexcept {}
std::vector<ConsoleLogLine> snapshot_after(std::uint64_t) { return {}; }
std::uint64_t latest_sequence() noexcept { return 0; }
}
