#include "crash_report.h"
#include "trace.h"
namespace dingosdk::crash_report {
StartResult start(const std::filesystem::path&) noexcept { return StartResult::disabled; }
void stop() noexcept {}
void capture(EXCEPTION_POINTERS*) noexcept {}
void capture_game_fatal(DWORD, EXCEPTION_POINTERS*) noexcept {}
void set_destination(std::wstring_view) noexcept {}
void set_gpu(std::wstring_view, std::wstring_view) noexcept {}
}
namespace dingosdk::trace {
bool start() noexcept { return false; }
bool active() noexcept { return false; }
void record(std::string_view, std::string_view, std::chrono::milliseconds, bool, Tags, Measurements) noexcept {}
void frame(std::uint64_t) noexcept {}
FramePacing take_frame_pacing() noexcept { return {}; }
void flush(std::chrono::milliseconds) noexcept {}
std::string envelope(const Transaction&) { return {}; }
std::wstring envelope_url(std::wstring_view) { return {}; }
}
