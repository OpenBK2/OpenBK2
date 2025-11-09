#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

void InitLogging() {
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("msvc_logger", msvc_sink);
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
}
