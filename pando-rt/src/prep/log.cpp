// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "log.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "spdlog/fmt/bundled/compile.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "index.hpp"
#include "nodes.hpp"

namespace pando {

namespace {

const char loggerName[] = "pando-rt";

// Custom formatter flag that adds the node index
class NodeFormatterFlag : public spdlog::custom_flag_formatter {
public:
  void format(const spdlog::details::log_msg&, const std::tm&,
              spdlog::memory_buf_t& dest) override {
    auto nodeIndexText = fmt::format(FMT_COMPILE("{}"), Nodes::getCurrentNode());
    dest.append(nodeIndexText.data(), nodeIndexText.data() + nodeIndexText.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<NodeFormatterFlag>();
  }
};

} // namespace

[[nodiscard]] Status Logger::initialize() {
  using std::string_view_literals::operator""sv;

  if (spdlog::default_logger_raw()->name() != loggerName) {
    // formatter that adds node index
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<NodeFormatterFlag>('*').set_pattern(
        "[%Y-%m-%d %H:%M:%S.%f] [%n:%*] [%^%l%$] [%s:%#] %v");
    // replace default logger
    auto logger = spdlog::stdout_color_mt(loggerName);
    logger->set_formatter(std::move(formatter));
    spdlog::set_default_logger(std::move(logger));
  }

  if (const char* logLevelVar = std::getenv("PANDO_PREP_LOG_LEVEL"); logLevelVar != nullptr) {
    const std::string_view logLevel(logLevelVar);
    if (logLevel == "info"sv) {
      spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warning"sv) {
      spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error"sv) {
      spdlog::set_level(spdlog::level::err);
    } else {
      SPDLOG_ERROR("Unsupported logging level: {}", logLevel);
      return Status::Error;
    }
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  SPDLOG_INFO("Logging initialized");

  return Status::Success;
}

} // namespace pando
