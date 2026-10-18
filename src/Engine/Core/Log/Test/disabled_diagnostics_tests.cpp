#include "Engine/Core/Log/logging.h"
#include "Engine/Core/Profiling/profiler.h"
#include "Engine/Core/Debug/crash_report.h"
#include "Engine/Core/Debug/trace.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Expected fixture root.");
        const auto directory=std::filesystem::path(argv[1])/std::to_string(GetCurrentProcessId());
        if(std::filesystem::exists(directory))throw std::runtime_error("Fixture must not exist.");
        dingosdk::logging::Options options;options.directory=directory;options.level=dingosdk::logging::Level::trace;options.external_console=true;
        if(!dingosdk::logging::initialize(options))throw std::runtime_error("Disabled logger initializes without output.");
        dingosdk::logging::startup_banner();
        dingosdk::logging::log(dingosdk::logging::Level::critical,dingosdk::logging::Channel::ui,"No output {}",123);
        dingosdk::logging::event(dingosdk::logging::Channel::ui,"{\"probe\":true}");
        dingosdk::logging::printf(dingosdk::logging::Level::debug,dingosdk::logging::Channel::runtime,"No output %d",456);
        dingosdk::logging::set_level(dingosdk::logging::Level::trace);
        dingosdk::logging::flush();
        if(dingosdk::logging::enabled(dingosdk::logging::Level::critical) || dingosdk::logging::status().file_output ||
           dingosdk::logging::status().external_console || dingosdk::logging::latest_sequence() ||
           !dingosdk::logging::snapshot_after(0).empty() || std::filesystem::exists(directory))
            throw std::runtime_error("Lean build must not create log files, console sinks or recorded events.");
        dingosdk::profiler::set_hud(true);dingosdk::profiler::set_window(true);
        std::string error;
        if(dingosdk::profiler::active() || dingosdk::profiler::start_sampling({},error))throw std::runtime_error("No profiling workers in the playtest build.");
        SetEnvironmentVariableW(L"RESKATE_TRACING",L"1");
        SetEnvironmentVariableW(L"RESKATE_CRASH_REPORTING",L"1");
        if(dingosdk::crash_report::start(directory)!=dingosdk::crash_report::StartResult::disabled ||
           dingosdk::trace::start() || dingosdk::trace::active())
            throw std::runtime_error("Lean 2.1.0 must not enable crash uploads or tracing from environment settings.");
        dingosdk::trace::record("test","test",std::chrono::milliseconds(1));
        dingosdk::trace::flush(std::chrono::milliseconds(1));
        dingosdk::crash_report::stop();
        if(std::filesystem::exists(directory))throw std::runtime_error("Disabled 2.1.0 reporting created output.");
        dingosdk::logging::shutdown();
        std::cout<<"Diagnostics stay disabled even when trace, console and profiling are requested.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
