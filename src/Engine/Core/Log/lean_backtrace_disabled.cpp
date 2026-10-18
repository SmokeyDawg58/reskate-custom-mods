// DINGOSDK_LEAN_LOGGING diagnostic builds: the lean runtime keeps its log file but
// still ships no crash reporter, so the logger's backtrace calls do nothing.
#include "Engine/Core/Debug/backtrace.h"

namespace dingosdk::backtrace {
StartResult start(const std::filesystem::path &) noexcept { return StartResult::disabled; }
void stop() noexcept {}
void capture(EXCEPTION_POINTERS *) noexcept {}
}
