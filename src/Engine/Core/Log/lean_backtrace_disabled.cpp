// DINGOSDK_LEAN_LOGGING diagnostic builds: the lean runtime keeps its log file but
// still ships no crash reporter, so the logger's backtrace calls do nothing. (Builds
// without the backtrace module, as from 2.1.0, need nothing here.)
#if __has_include("Engine/Core/Debug/backtrace.h")
#include "Engine/Core/Debug/backtrace.h"

namespace dingosdk::backtrace {
StartResult start(const std::filesystem::path &) noexcept { return StartResult::disabled; }
void stop() noexcept {}
void capture(EXCEPTION_POINTERS *) noexcept {}
}
#endif
