#include <boost/asio/error.hpp>
#include <boost/system/detail/error_code.hpp>

#include "overwrite_log_macros.h"
#include "quill_static.h"
#define BOOST_STACKTRACE_USE_BACKTRACE 1
#define BOOST_ASIO_HAS_FILE 1
#define BOOST_ASIO_HAS_IO_URING 1
#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/file_base.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/stream_file.hpp>
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
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <re2/re2.h>

#include "gyou/array_utils.hpp"
#include "gyou/boost_stacktrace_format.hpp"
#include "gyou/http_requests.hpp"
#include "gyou/omega_exception.hpp"

// it is for getting real type from compiler when debugging via
// `Debug<a_type_i_dont_know_and_i_want_understand> sth;`
template <typename T> struct Debug;

constexpr size_t DEFAULT_MAX_CONCURRENT_REQUESTS_PER_SERVICE = 6;

// NOLINTNEXTLINE(performance-enum-size)
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

// Fucking C++ without C99 designated array initializer extension makes me do
// this shit.
constexpr auto ServicesNames = std::invoke(
    []()
        {
            constexpr auto not_a_map_because_of_fucking_cpp = std::to_array({
                // clang-format off
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::bitbucket),"https://bitbucket.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::codeberg),"https://codeberg.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::cpan),"https://metacpan.org/dist/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::cpan_module),"https://metacpan.org/pod/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::cpe),"cpe:/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::cran),"https://cran.r-project.org/web/packages/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::ctan),"https://ctan.org/pkg/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::freedesktop_gitlab),"https://gitlab.freedesktop.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::gentoo),"https://gitweb.gentoo.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::github),"https://github.com/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::gitlab),"https://gitlab.com/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::gnome_gitlab),"https://gitlab.gnome.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::google_code),"https://code.google.com/archive/p/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::hackage),"https://hackage.haskell.org/package/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::heptapod),"https://foss.heptapod.net/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::kde_invent),"https://invent.kde.org/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::launchpad),"https://launchpad.net/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::osdn),"https://osdn.net/projects/.*/"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::pear),"https://pear.php.net/package/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::pecl),"https://pecl.php.net/package/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::pypi),"https://pypi.org/project/.*/"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::rubygems),"https://rubygems.org/gems/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::savannah),"https://savannah.gnu.org/projects/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::savannah_nongnu),"https://savannah.nongnu.org/projects/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::sourceforge),"https://sourceforge.net/projects/.*/"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::sourcehut),"https://sr.ht/.*"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Services::vim),"https://www.vim.org/scripts/script.php?script_id=.*")
                // clang-format on
            });

            std::array<std::string_view,
                       std::size(not_a_map_because_of_fucking_cpp)>
                arr;

            for (size_t i = 0; i < std::size(not_a_map_because_of_fucking_cpp);
                 ++i)
                {
                    arr.at(i) = not_a_map_because_of_fucking_cpp.at(i).second;
                }

            return arr;
        });

struct Config
{
    boost::beast::http::fields headers;
    std::filesystem::path log_file;
    std::filesystem::path path_to_repo;
    std::filesystem::path path_to_portage_bin;
    std::filesystem::path path_to_portage_temp;
    std::filesystem::path path_to_portage_pym;
    std::filesystem::path path_to_gentoo_repo;
    quill::LogLevel log_level{};
    size_t concurrency_per_service{};
};

struct EntryData
{
    std::string link_str;
    std::string author;
    std::string title;
    std::string description;
};

// NOLINTNEXTLINE(performance-enum-size)
enum class ReturnCode : int
{
    Success = 0,
    PartialSuccess = 1,
    AllHaveFailed = 2,
    NoEbuildFound = 3,
    FailDuringParsingCmdValues = 4,
    FailSpecifiedValueIsIncorrect = 5,
    FailDuringInitializationConfig = 6,
    FailStandardException = 7,
    FailParsePromptResult = 8,
    FailInitializationLogger = 9,
    ReceivedCancellationSignal = 10,

};

enum class PackageType
{
    Unknown,
    ReleaseOrTag,
    Commit,
};

namespace
{

    struct DataFromEbuild
    {
        std::string src_uri_str;
        std::optional<std::string> commit;
    };

    struct EntryToChange
    {
        std::string old_name;
        std::string new_name;

        std::optional<std::string> commit;
    };

    [[nodiscard]] corral::Task<
        std::expected<std::string, boost::system::error_code>>
    file_to_string(auto& ioc, std::filesystem::path const& file_path)
    {
        boost::asio::stream_file file_reader(
            ioc, file_path, boost::asio::stream_file::read_only);

        std::string str_of_file;
        str_of_file.reserve(file_reader.size());

        auto&& [errc, bytes_read] = co_await boost::asio::async_read(
            file_reader, boost::asio::buffer(str_of_file),
            corral::asio_nothrow_awaitable);
        if (errc && boost::asio::error::eof != errc)
            {
                co_return std::unexpected(errc);
            }
        co_return str_of_file;
    }

    [[nodiscard]] corral::Task<
        std::expected<std::filesystem::path, std::string>>
    bash_ebuild(auto& ioc, Config const& cfg,
                std::filesystem::path const& path_to_ebuild)
    {
        std::string pkg_name = path_to_ebuild.filename().stem().string();
        std::string category = path_to_ebuild.parent_path().filename().string();

        std::filesystem::path temp_folder
            = (cfg.path_to_portage_temp / category / pkg_name).concat("/");

        std::unordered_map<boost::process::environment::key,
                           boost::process::environment::value>
            env_for_ebuild = {
                {"EBUILD", path_to_ebuild.string()},
                {"T", temp_folder.string()},
                {"PORTAGE_BIN_PATH", cfg.path_to_portage_bin.string()},
                {"PORTAGE_PYM_PATH", cfg.path_to_portage_pym.string()},
                {"PORTAGE_ECLASS_LOCATIONS_STR",
                 cfg.path_to_gentoo_repo.string() + ":"
                     + cfg.path_to_repo.string()},
                {"EBUILD_PHASE", "_internal_test"},
                {"CATEGORY", category},
                {"PF", pkg_name},
            };

        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        std::string shell_args
            = (cfg.path_to_portage_bin / "ebuild.sh").string()
              + " _internal_test " + path_to_ebuild.string();

        boost::process::shell cmd_get_subtitles
            = boost::process::shell(shell_args);

        auto exe = cmd_get_subtitles.exe();
        auto proc = boost::process::process(
            ioc, exe, cmd_get_subtitles.args(),
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr});

        auto read_loop = [](boost::asio::readable_pipe& p) -> corral::Task<std::string>
            {
                std::string res;
                std::array<char, 4096> buf;
                for (;;)
                    {
                        auto [error_code, received_size]
                            = co_await p.async_read_some(
                                boost::asio::buffer(buf),
                                corral::asio_nothrow_awaitable);
                        if (received_size)
                            {
                                res.append(buf.data(), received_size);
                            }
                        if (error_code)
                            {
                                co_return res;
                            }
                    }
            };

        auto wait_proc = [](boost::process::process& p) -> corral::Task<int>
            {
                auto [ec, exit_code]
                    = co_await p.async_wait(corral::asio_nothrow_awaitable);
                co_return exit_code;
            };

        LOG_DEBUG("Doing sth in bash: {}", shell_args);

        auto [errc_proc, stdout_str, stderr_str] = co_await corral::allOf(
            wait_proc(proc), read_loop(rp_stdout), read_loop(rp_stderr));

        if (not stderr_str.empty() || errc_proc != 0)
            {
                co_return std::unexpected(fmt::format(
                    "Failed to do yt-dlp: ec: {}\nstderr: {}\nstdout: {}",
                    errc_proc, stderr_str, stdout_str));
            }

        co_return temp_folder;
    }

    // get current version of the pkg or commit of
    // current pkg extract service and link
    // check if we support the service
    // semaphore
    // check for update
    //  return what to change
    [[nodiscard]] corral::Task<std::expected<DataFromEbuild, std::string>>
    logic_per_ebuild(auto& ioc, Config const& cfg, PackageType pkg_type,
                     std::filesystem::path const& path_to_ebuild,
                     auto& semaphores, RE2 const& re_commit_str,
                     RE2 const& re_src_uri, uint64_t date)
    {
        auto temp_folder_path_res
            = co_await bash_ebuild(ioc, cfg, path_to_ebuild);

        if (!temp_folder_path_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to run ebuild.sh: {}",
                                temp_folder_path_res.error()));
            }
        std::filesystem::path temp_file_path
            = (temp_folder_path_res.value() / "temp");

        auto str_file_res = co_await file_to_string(ioc, temp_file_path);
        if (not str_file_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to read file. error: {}",
                                str_file_res.error().message()));
            }
        std::string_view commit_env_var_name;

        std::string_view commit;
        if (PackageType::Commit == pkg_type)
            {
                if (not RE2::FullMatch(str_file_res.value(), re_commit_str,
                                       &commit_env_var_name, &commit))
                    {
                        co_return std::unexpected(
                            fmt::format("Regex for getting commit failed: {}",
                                        re_commit_str.error()));
                    };
            }

        // get first url in SRC_URI
        std::string_view src_uri_str;

        if (not RE2::FullMatch(str_file_res.value(), re_src_uri, &src_uri_str))
            {
                co_return std::unexpected(fmt::format(
                    "Regex for getting first URL from SRC_URI failed: {}",
                    re_src_uri.error()));
            }

        // understand what service is this
    }

    [[nodiscard]] corral::Task<ReturnCode> chief_logic(auto& ioc,
                                                       Config const& cfg,
                                                       auto& semaphores)
    {
        // to compile them all at once
        RE2 re_commit_str(
            R"delimiter(declare -- ([a-zA-Z_]?[a-zA-Z0-9_]*?COMMIT[a-zA-Z0-9_]*?)="([0-9a-f]{40})"\n)delimiter",
            RE2::Quiet);
        RE2 re_src_uri(
            R"delimiter(declare SRC_URI="(https?://\S*)(?: -> \S* ?.*)?"\n)delimiter",
            RE2::Quiet);
        RE2 re_category(R"(([\w][\w+.-]*))", RE2::Quiet);
        RE2 re_pkg_9999(R"([\w+.-]*9999)", RE2::Quiet);
        RE2 re_pkg_with_date(R"([\w+.-]+?(\d{8})[\w+.-]*?)", RE2::Quiet);

        std::vector<DataFromEbuild> bash_res;
        bool is_any_successful = false;
        bool is_any_failed = false;

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

                                    auto pkg_type = PackageType::Unknown;

                                    uint64_t date = 0;

                                    if (RE2::FullMatch(pkg_pv, re_pkg_with_date,
                                                       &date))
                                        {
                                            pkg_type = PackageType::Commit;
                                            LOG_INFO(
                                                "This is a package per "
                                                "commit: "
                                                "{} . Its date: {}",
                                                pkg_pv, &date);
                                        }
                                    else
                                        {
                                            pkg_type
                                                = PackageType::ReleaseOrTag;
                                            LOG_INFO(
                                                "This is a package per "
                                                "release: {}",
                                                pkg_pv);
                                        }

                                    nursery.start(
                                        [&](PackageType pkg_type_arg,
                                            std::filesystem::path
                                                file_arg) mutable
                                            -> corral::Task<void>
                                            {
                                                auto sth
                                                    = co_await logic_per_ebuild(
                                                        ioc, cfg, pkg_type_arg,
                                                        file_arg, semaphores,
                                                        re_commit_str,
                                                        re_src_uri, date);
                                                if (!sth)
                                                    {
                                                        is_any_failed = true;
                                                        LOG_ERROR(
                                                            "Failed to do "
                                                            "sth "
                                                            "with a "
                                                            "ebuild: {}",
                                                            sth.error());
                                                        co_return;
                                                    }
                                                bash_res.emplace_back(
                                                    sth.value());
                                                is_any_successful = true;
                                            },
                                        pkg_type, file);
                                }
                        }
                }

            co_return corral::join;
        };
        LOG_INFO("Writing result to stdout...");
        // std::stringstream strs;
        // boost::property_tree::write_xml(strs, tree);
        LOG_INFO("Wrote result to stdout.");
        // co_return strs.str();
        if (is_any_failed and is_any_successful)
            {
                co_return ReturnCode::PartialSuccess;
            }
        else if (is_any_failed and not is_any_successful)
            {
                co_return ReturnCode::AllHaveFailed;
            }
        else if (not is_any_successful)
            {
                co_return ReturnCode::NoEbuildFound;
            }
        else
            {
                co_return ReturnCode::Success;
                ;
            }
    }

    boost::property_tree::ptree parse_rss_into_tree(std::string const& rss_feed)
    {
        // LOG_DEBUG("Waiting for YouTube's RSS feed from stdin...");
        // std::cin >> std::noskipws;
        // std::istreambuf_iterator<char> start(std::cin);
        // std::istreambuf_iterator<char> end;
        // std::string xml_rss_youtube_feed(start, end);
        // LOG_DEBUG("Received the YouTube's RSS feed.");
        //
        // boost::property_tree::ptree tree
        //     = parse_rss_into_tree(xml_rss_youtube_feed);
        LOG_DEBUG("Received something from stdin...");
        LOG_TRACE_L1("rss_feed: {}", rss_feed);
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        LOG_DEBUG("Trying to parse it as an XML...");
        boost::property_tree::read_xml(istr, tree);
        LOG_DEBUG("Successfully parsed an XML...");
        return tree;
    }

    corral::Task<ReturnCode> actual_chief(auto& ioc, Config const& cfg)
    {
        constexpr size_t amount_of_services
            = magic_enum::enum_count<Services>();

        std::array<corral::Semaphore, amount_of_services>
            semaphores_per_services
            = make_array_from_factory<amount_of_services>(
                [concurrency_per_service = cfg.concurrency_per_service]()
                    { return corral::Semaphore(concurrency_per_service); });

        ReturnCode res
            = co_await chief_logic(ioc, cfg, semaphores_per_services);
        co_return res;
    }

}  // namespace

int main(int argc, char* argv[])
{
    Config cfg;

    // Config parsing
    {
        CLI::App app{
            "Post-processor for YouTube's RSS feed, so that you get "
            "summary of video inside the feed via sending an HTTP "
            "request to something like an Ollama instance."};

        // Temporary storage for CLI11 to map types it doesn't handle
        // natively without custom validators
        std::vector<std::string> headers_raw
            = {"Content-Type: application/json"};
        std::string log_level_str = "info";

        app.add_option("-H,--header", headers_raw,
                       "HTTP headers for request to an ?Ollama? instance.")
            ->take_all();

        app.add_option("-l,--log-file", cfg.log_file,
                       "Filepath to internal logs")
            ->default_val("./logs.log");

        app.add_option("-R,--repo-path", cfg.path_to_repo, "Filepath to repo")
            ->required();

        app.add_option("-t,--tmp-path", cfg.path_to_portage_temp,
                       "Filepath to portage's tmp")
            ->required();

        app.add_option("-G,--gentoo-repo-path", cfg.path_to_gentoo_repo,
                       "Filepath to gentoo's repo path")
            ->default_val("/var/db/repos/gentoo/");

        app.add_option("-P,--portage-bin-path", cfg.path_to_portage_bin,
                       "Filepath to portage's bin folder i.e. `git clone "
                       "https://github.com/Arniiiii/portage` and give the path "
                       "to `./bin/` folder")
            ->required();

        app.add_option("-Y,--portage-pym-path", cfg.path_to_portage_pym,
                       "Filepath to portage's PYM folder. I have no idea what "
                       "this is, but it can be any empty folder. ")
            ->required();

        app.add_option("--log-level", log_level_str,
                       "Log level: "
                       "tracel3,tracel2,tracel1,debug,info,notice,warning,"
                       "error,critical")
            ->default_val("info");

        app.add_option("-J,--jobs-requests", cfg.concurrency_per_service,
                       "Amount of concurrent request per service"
                       "by this application")
            ->check(CLI::PositiveNumber)
            ->default_val(DEFAULT_MAX_CONCURRENT_REQUESTS_PER_SERVICE);
        try
            {
                app.parse(argc, argv);

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
                    ReturnCode::FailSpecifiedValueIsIncorrect);
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
                    ReturnCode::FailDuringInitializationConfig);
            }
    }

    // Enabling logging
    try
        {
            setup_quill(cfg.log_file, cfg.log_level);
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
            return std::to_underlying(ReturnCode::FailInitializationLogger);
        }

    // Finally main.
    try
        {
            boost::asio::io_context ioc;
            LOG_DEBUG("Entering asynchronous main...");
            boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
            auto&& [main_opt, signals_opt] = corral::run(
                ioc, corral::anyOf(actual_chief(ioc, cfg),
                                   signals.async_wait(corral::asio_awaitable)));
            if (main_opt)
                {
                    return std::to_underlying(main_opt.value());
                }
            return std::to_underlying(ReturnCode::ReceivedCancellationSignal);
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
            return std::to_underlying(ReturnCode::FailParsePromptResult);
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
            return std::to_underlying(ReturnCode::FailStandardException);
        }
}
