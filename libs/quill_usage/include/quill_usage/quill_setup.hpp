#pragma once

#include <filesystem>

#include <quill/core/LogLevel.h>
void setup_quill(std::filesystem::path const& log_file,
                 quill::LogLevel log_level);
