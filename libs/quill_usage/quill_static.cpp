#include "quill_static.hpp"

#include <quill/core/LogLevel.h>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

// Define a global variable for a logger to avoid looking up the logger each
// time. Additional global variables can be defined for additional loggers if
// needed.
quill::Logger* global_logger_a;

void setup_quill(std::filesystem::path const& log_file,
                 quill::LogLevel log_level)
{
    // Start the backend thread
    quill::BackendOptions backend_options;
    backend_options.check_printable_char = {};
    quill::Backend::start(backend_options);

    if (log_file == "console")
        {
            // Setup sink and logger
            auto console_sink
                = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                    "console_log_id_1");
            global_logger_a = quill::Frontend::create_or_get_logger(
                "root", std::move(console_sink));
        }
    else
        {
            // Setup sink and logger
            auto file_sink
                = quill::Frontend::create_or_get_sink<quill::FileSink>(
                    log_file,
                    []()
                        {
                            quill::FileSinkConfig config_quill;
                            config_quill.set_open_mode('w');
                            // config_quill.set_filename_append_option (
                            //     quill::FilenameAppendOption::StartDateTime);
                            return config_quill;
                        }(),
                    quill::FileEventNotifier{});
            global_logger_a = quill::Frontend::create_or_get_logger(
                "gyou", std::move(file_sink)
                // ,quill::PatternFormatterOptions{
                //             "%(time) [%(thread_id)]
                //             %(short_source_location:<28) "
                //             "LOG_%(log_level:<9) %(logger:<12) %(message)",
                //             "%H:%M:%S.%Qns", quill::Timezone::GmtTime}
            );
        }
    // Create and store the logger
    global_logger_a->set_log_level(log_level);
}
