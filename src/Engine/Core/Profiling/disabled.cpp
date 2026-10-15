#include "profiler.h"

namespace dingosdk::profiler {
std::atomic<bool> zones_enabled{false};
Site::Site(const char* value) noexcept : name(value) {}
void record_client_update(std::uint64_t) noexcept {}
void record_client_frame(std::uint64_t) noexcept {}
void record_present() noexcept {}
std::size_t history(Series, std::span<float>) noexcept { return 0; }
void set_hud(bool) noexcept {}
bool hud() noexcept { return false; }
void set_window(bool) noexcept {}
bool window() noexcept { return false; }
bool active() noexcept { return false; }
std::shared_ptr<const Summary> summary() noexcept { return {}; }
bool start_sampling(const SampleRequest&, std::string& error) { error="Profiling is not included in this playtest build."; return false; }
void stop_sampling() noexcept {}
std::shared_ptr<const SampleReport> sample_report() noexcept { return {}; }
void set_engine_labels(std::span<const Label>) noexcept {}
}
