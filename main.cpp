

#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields_fwd.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/shell.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/stacktrace.hpp>
#include <boost/url.hpp>
#include <corral/Nursery.h>
#include <corral/Semaphore.h>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>
#include <corral/run.h>
#include <corral/wait.h>
#include <experimental/array>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <glaze/glaze.hpp>
#include <inja/environment.hpp>
#include <inja/inja.hpp>
#include <inja/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/core/LogLevel.h>
#include <quill/sinks/FileSink.h>
#include <re2/re2.h>

#include "gyou/boost_stacktrace_format.hpp"
#include "gyou/http_requests.hpp"
#include "gyou/omega_exception.hpp"

template <typename T> struct Debug;

extern quill::Logger* global_logger_a;

constexpr size_t MAX_CONCURRENT_OLLAMA_DEFAULT = 6;

namespace beast = boost::beast;
namespace net = boost::asio;

enum class Services : size_t
{
    github = 0,
    gitlab = 1,
    bitbucket = 2,
    codeberg = 3,
    cpan = 4,
    cpan_module = 5,
    cpe = 6,
    cran = 7,
    ctan = 8,
    freedesktop_gitlab = 9,
    gentoo = 10,
    gnome_gitlab = 11,
    google_code = 12,
    hackage = 13,
    heptapod = 14,
    kde_invent = 15,
    launchpad = 16,
    osdn = 17,
    pear = 18,
    pecl = 19,
    pypi = 20,
    rubygems = 21,
    savannah = 22,
    savannah_nongnu = 23,
    sourceforge = 24,
    sourcehut = 25,
    vim = 26,
};

struct Config
{
    boost::url url;
    beast::http::verb method;
    beast::http::fields headers;
    std::filesystem::path log_file;
    quill::LogLevel log_level;
    size_t concurrency_per_service{};
    std::filesystem::path path_to_repo;
};

struct EntryData
{
    std::string link_str;
    std::string author;
    std::string title;
    std::string description;
};

struct RequestServer
{
    std::string url;
};
// NOLINTNEXTLINE(performance-enum-size)
enum class ReturnCodes : int
{
    Success = 0,
    FailDuringParsingCmdValues = 1,
    FailSpecifiedValueIsIncorrect = 2,
    FailDuringInitializationConfig = 3,
    FailCacheFolder = 4,
    FailStandardException = 5,
    FailParsePromptResult = 6,
    FailInitializationLogger = 7,

};

enum class PackageType
{
    Unknown,
    Release,
    Commit,
};

namespace
{

    template <std::size_t N, typename F> auto make_array_from_factory(F f)
        -> std::array<std::decay_t<decltype(f())>, N>
    {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                return std::array<std::decay_t<decltype(f())>, N>{
                    {(static_cast<void>(Is), f())...}};
            }(std::make_index_sequence<N>());
    }

    corral::Task<std::expected<std::string, std::string>> request_to_LLM(
        auto& ioc, std::string& request_body, Config const& cfg)
    {
        if ("https" == cfg.url.scheme())
            {
                co_return co_await typical_https_request(
                    ioc, request_body, cfg.url, cfg.method, cfg.headers);
            }
        else
            {
                co_return co_await typical_http_request(
                    ioc, request_body, cfg.url, cfg.method, cfg.headers);
            }
    }

    corral::Task<std::string> main_logic(auto& ioc,
                                         boost::property_tree::ptree tree,
                                         Config const& cfg, auto& semaphores)
    {
        RE2 re_category(R"(([\w][\w+.-]*))", RE2::Quiet);
        RE2 re_pkg_9999(R"([\w+.-]*9999)", RE2::Quiet);
        RE2 re_pkg_with_date(R"([\w+.-]+?(\d{8})[\w+.-]*?)", RE2::Quiet);

        CORRAL_WITH_NURSERY(nursery)
        {
            for (const auto& category :
                 std::filesystem::directory_iterator(cfg.path_to_repo))
                {
                    if (not category.is_directory())
                        {
                            continue;
                        }
                    std::string category_str
                        = category.path().filename().string();
                    if (not RE2::FullMatch(category_str, re_category))
                        {
                            continue;
                        }

                    for (const auto& pkg_name :
                         std::filesystem::directory_iterator(category))
                        {
                            if (not pkg_name.is_directory())
                                {
                                    continue;
                                }

                            PackageType pkg_type = PackageType::Unknown;

                            for (const auto& file :
                                 std::filesystem::directory_iterator(pkg_name))
                                {
                                    if (not file.is_regular_file())
                                        {
                                            continue;
                                        }

                                    std::filesystem::path pkg_filename
                                        = file.path().filename();

                                    if (pkg_filename.extension() != ".ebuild")
                                        {
                                            continue;
                                        }

                                    std::string pkg_pv
                                        = pkg_filename.stem().string();

                                    if (RE2::FullMatch(pkg_pv, re_pkg_9999))
                                        {
                                            continue;
                                        }

                                    if (RE2::FullMatch(pkg_pv,
                                                       re_pkg_with_date))
                                        {
                                            // get commit of current pkg
                                            // extract service and link
                                            // check if we support the service
                                            // semaphore
                                            // check for update
                                        }

                                    // get current version of the pkg
                                    // extract service and link
                                    // check if we support the service
                                    // semaphore
                                }
                        }
                }

            co_return corral::join;
        };
        LOG_INFO("Writing result to stdout...");
        std::stringstream strs;
        boost::property_tree::write_xml(strs, tree);
        LOG_INFO("Wrote result to stdout.");
        co_return strs.str();
    }

    boost::property_tree::ptree parse_rss_into_tree(std::string const& rss_feed)
    {
        LOG_DEBUG("Received something from stdin...");
        LOG_TRACE_L1("rss_feed: {}", rss_feed);
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        LOG_DEBUG("Trying to parse it as an XML...");
        boost::property_tree::read_xml(istr, tree);
        LOG_DEBUG("Successfully parsed an XML...");
        return tree;
    }

    corral::Task<void> actual_main(auto& ioc, Config const& cfg)
    {
        LOG_DEBUG("Waiting for YouTube's RSS feed from stdin...");
        std::cin >> std::noskipws;
        std::istreambuf_iterator<char> start(std::cin);
        std::istreambuf_iterator<char> end;
        std::string xml_rss_youtube_feed(start, end);
        LOG_DEBUG("Received the YouTube's RSS feed.");

        boost::property_tree::ptree tree
            = parse_rss_into_tree(xml_rss_youtube_feed);

        constexpr size_t amount_of_services
            = magic_enum::enum_count<Services>();

        std::array<corral::Semaphore, amount_of_services>
            semaphores_per_services
            = make_array_from_factory<amount_of_services>(
                [concurrency_per_service = cfg.concurrency_per_service]()
                    { return corral::Semaphore(concurrency_per_service); });

        co_await main_logic(ioc, tree, cfg, semaphores_per_services);
    }

}  // namespace

int main(int argc, char* argv[])
{
    Config cfg;

    {
        CLI::App app{
            "Post-processor for YouTube's RSS feed, so that you get "
            "summary of video inside the feed via sending an HTTP "
            "request to something like an Ollama instance."};

        // Temporary storage for CLI11 to map types it doesn't handle
        // natively without custom validators
        std::string url_str = "http://127.0.0.1:11434/api/chat";
        std::string method_str = "post";
        std::vector<std::string> headers_raw
            = {"Content-Type: application/json"};
        std::string log_level_str = "info";

        app.add_option("-u,--url", url_str,
                       "URL of ?Ollama? instance in format "
                       "http://127.0.0.1:11434/api/chat")
            ->capture_default_str();

        app.add_option("-X,--method", method_str,
                       "HTTP method by which to ask an ?Ollama? instance. "
                       "Possible values: get, post, head, patch, purge etc.")
            ->capture_default_str();

        app.add_option("-H,--header", headers_raw,
                       "HTTP headers for request to an ?Ollama? instance.")
            ->take_all();

        app.add_option("-l,--log-file", cfg.log_file,
                       "Filepath to internal logs")
            ->default_val("./logs.log");

        app.add_option("-R,--repo-path", cfg.path_to_repo, "Filepath to repo");

        app.add_option("--log-level", log_level_str,
                       "Log level: "
                       "tracel3,tracel2,tracel1,debug,info,notice,warning,"
                       "error,critical")
            ->default_val("info");

        app.add_option("-J,--jobs-requests", cfg.concurrency_per_service,
                       "Amount of concurrent request per service"
                       "by this application")
            ->check(CLI::PositiveNumber)
            ->default_val(MAX_CONCURRENT_OLLAMA_DEFAULT);
        try
            {
                app.parse(argc, argv);

                // Post-processing complex types
                cfg.url = boost::urls::url(url_str);

                auto verb_opt
                    = magic_enum::enum_cast<beast::http::verb>(method_str);
                if (!verb_opt)
                    {
                        throw CLI::ValidationError(
                            "method", "Invalid HTTP method: " + method_str);
                    }
                cfg.method = *verb_opt;

                cfg.log_level = quill::loglevel_from_string(log_level_str);

                for (const auto& header_raw_str : headers_raw)
                    {
                        std::vector<std::string> parts;
                        boost::split(parts, header_raw_str,
                                     boost::is_any_of(":"));
                        if (parts.size() >= 2)
                            {
                                std::string key = boost::trim_copy(parts[0]);
                                std::string val = boost::trim_copy(parts[1]);
                                cfg.headers.insert(key, val);
                            }
                    }
            }
        catch (const CLI::ParseError& e)
            {
                return app.exit(e);
            }
        catch (OmegaException<std::string>& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                std::string log = fmt::format(
                    "Oohh, look at you, who got an exception, my cutie "
                    "lovely "
                    "guy. "
                    "\nThis is an Omega exception. Most definitely an "
                    "incorrect "
                    "value was specified in args. Here's "
                    ".what():\n\n{}\nHere's data:\n{}\n Here's "
                    "trace:\n{}\nHere's "
                    "line where something failed:\n{}\n\nHere's an attempt "
                    "to "
                    "get "
                    "backtrace of it "
                    "via "
                    "boost::stacktrace and libbacktrace...",
                    e.what(), e.data(), e.stack(), e.where(),
                    boost::stacktrace::to_string(trace));

                fmt::print(std::cerr, "{}", log);
                return std::to_underlying(
                    ReturnCodes::FailSpecifiedValueIsIncorrect);
            }
        catch (std::exception& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                fmt::print(std::cerr,
                           "Oohh, look at you, who got an exception, my cutie "
                           "lovely guy. "
                           "\nYou know that standard exceptions sucks. Here's "
                           ".what():\n\n{}\n\nHere's an attempt to get "
                           "backtrace of it "
                           "via "
                           "boost::stacktrace and libbacktrace... Hope it "
                           "works or kill "
                           "youself debugging this shit.\n{}\n",
                           e.what(), boost::stacktrace::to_string(trace));
                return std::to_underlying(
                    ReturnCodes::FailDuringInitializationConfig);
            }
    }

    try
        {
            // Setup sink and logger
            auto file_sink
                = quill::Frontend::create_or_get_sink<quill::FileSink>(
                    cfg.log_file,
                    []()
                        {
                            quill::FileSinkConfig config_quill;
                            config_quill.set_open_mode('w');
                            // config_quill.set_filename_append_option (
                            //     quill::FilenameAppendOption::StartDateTime);
                            return config_quill;
                        }(),
                    quill::FileEventNotifier{});

            // Create and store the logger
            global_logger_a = quill::Frontend::create_or_get_logger(
                "root", std::move(file_sink));
            global_logger_a->set_log_level(cfg.log_level);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            fmt::print(std::cerr,
                       "Oohh, look at you, who got an exception, my cutie "
                       "lovely guy. "
                       "\nYou know that standard exceptions sucks. Here's "
                       ".what():\n\n{}\n\nHere's an attempt to get "
                       "backtrace of it "
                       "via "
                       "boost::stacktrace and libbacktrace... Hope it "
                       "works or kill "
                       "youself debugging this shit.\n{}\n",
                       e.what(), boost::stacktrace::to_string(trace));
            return std::to_underlying(ReturnCodes::FailInitializationLogger);
        }

    try
        {
            quill::BackendOptions backend_options;
            backend_options.check_printable_char = {};
            quill::Backend::start(backend_options);

            LOG_DEBUG("Successfully parsed command line arguments.");

            LOG_INFO("Trying to parse supplied headers...");

            net::io_context ioc;
            LOG_DEBUG("Entering coroutine...");
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            corral::run(
                ioc, corral::anyOf(actual_main(ioc, cfg),
                                   signals.async_wait(corral::asio_awaitable)));
        }
    catch (OmegaException<std::filesystem::path>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely "
                "guy. "
                "\nThis is an Omega exception. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's "
                "trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to "
                "get "
                "backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace...",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(ReturnCodes::FailCacheFolder);
        }
    catch (OmegaException<std::string>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely "
                "guy. "
                "\nThis is an Omega exception. Maybe from parsing result "
                "from "
                "Ollama. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's "
                "trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to "
                "get "
                "backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace...",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(ReturnCodes::FailParsePromptResult);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely "
                "guy. "
                "\nYou know that standard exceptions sucks. Here's "
                ".what():\n\n{}\n\nHere's an attempt to get backtrace of "
                "it "
                "via "
                "boost::stacktrace and libbacktrace... Hope it works or "
                "kill "
                "youself debugging this shit.\n{}\n",
                e.what(), boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(ReturnCodes::FailStandardException);
        }
    return std::to_underlying(ReturnCodes::Success);
}
