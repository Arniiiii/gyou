

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
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/core/LogLevel.h>
#include <quill/sinks/FileSink.h>
#include <re2/re2.h>

#include "ytto/boost_stacktrace_format.hpp"
#include "ytto/cache.hpp"
#include "ytto/cache_file.hpp"
#include "ytto/ollama_parser.hpp"
#include "ytto/omega_exception.hpp"

template <typename T> struct Debug;

quill::Logger* logger;

constexpr auto HTTP_MAX_TIME_TIMEOUT_RFC = std::chrono::seconds(120);
constexpr auto MAX_PROMPT_TIME = std::chrono::minutes(10);
constexpr int HTTP_VERSION_TO_USE = 11;
constexpr size_t MAX_EXPECTED_CHARACTERS = 128000;
constexpr uint16_t SERVER_DEFAULT_PORT = 8000;
constexpr size_t MAX_CONCURRENT_YTDLP_DEFAULT = 5;
constexpr size_t MAX_CONCURRENT_OLLAMA_DEFAULT = 6;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

struct Config
{
    std::string language;
    std::string prompt_template;
    std::string http_body_template;
    boost::url url;
    beast::http::verb method;
    beast::http::fields headers;
    std::filesystem::path cache_file;
    std::filesystem::path cache_subtitles_file;
    std::filesystem::path log_file;
    quill::LogLevel log_level;
    size_t concurrency_yt_dlp{};
    size_t concurrency_ollama{};
    uint16_t server_port{};
    bool proceed_with_shorts{};
    bool enable_server{};
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

namespace
{

    corral::Task<std::expected<std::string, std::string>> typical_http_request(
        auto& ioc, std::string const& request_body, const boost::url& url,
        beast::http::verb method, beast::http::fields headers)
    {
        auto resolver = net::ip::tcp::resolver{ioc};

        LOG_DEBUG(logger,
                  "DNS look-up of an URL... "
                  "host: {} port:{}",
                  std::string(url.host()), std::string(url.port()));
        auto [ec_resolve, results] = co_await resolver.async_resolve(
            url.host(), url.port(), corral::asio_nothrow_awaitable);
        if (ec_resolve)
            {
                co_return std::unexpected(ec_resolve.message());
            }

        auto stream = beast::tcp_stream{ioc};
        stream.expires_after(std::chrono::seconds(HTTP_MAX_TIME_TIMEOUT_RFC));

        LOG_DEBUG(logger, "Trying to connect to an URL...");
        auto [ec_connect, ep] = co_await stream.async_connect(
            results, corral::asio_nothrow_awaitable);
        if (ec_connect)
            {
                co_return std::unexpected(ec_connect.message());
            }
        LOG_DEBUG(logger, "Successfully.");

        LOG_DEBUG(logger,
                  "Creating a request object... "
                  "method: {} path: {} http version:{}",
                  magic_enum::enum_name(method),
                  std::string(url.encoded_resource()), HTTP_VERSION_TO_USE);
        beast::http::request<http::string_body> request{
            method, url.encoded_resource(), HTTP_VERSION_TO_USE, request_body,
            headers};
        request.set(beast::http::field::host, url.host());
        request.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.prepare_payload();
        std::stringstream strs;
        strs << request;
        LOG_TRACE_L1(logger, "Request:\n{}", strs.str());
        stream.expires_after(MAX_PROMPT_TIME);

        LOG_INFO(logger, "Sending request to an LLM...");
        auto [ec_write, bytes_written] = co_await beast::http::async_write(
            stream, request, corral::asio_nothrow_awaitable);
        if (ec_write)
            {
                co_return std::unexpected(ec_write.message());
            }

        beast::flat_buffer buffer(MAX_EXPECTED_CHARACTERS);

        beast::http::response<http::string_body> response;

        LOG_INFO(logger, "Waiting for response...");
        auto [ec_read, bytes_read] = co_await beast::http::async_read(
            stream, buffer, response, corral::asio_nothrow_awaitable);
        if (ec_read)
            {
                co_return std::unexpected(ec_read.message());
            }
        LOG_INFO(logger, "Received response.");

        LOG_DEBUG(logger, "Trying to close connection.");

        beast::error_code error_code;
        stream.socket().shutdown(net::ip::tcp::socket::shutdown_both,
                                 error_code);

        if (error_code && error_code != beast::errc::not_connected)
            {
                co_return std::unexpected(error_code.message());
            }
        LOG_DEBUG(logger, "Supposedly closed connection.");

        if (response.result() != http::status::ok)
            {
                co_return std::unexpected("returned with status not 200");
            }

        co_return response.body();
    }

    corral::Task<std::expected<std::string, std::string>> typical_https_request(
        auto& ioc, std::string const& request_body, boost::url const& url,
        beast::http::verb method, const beast::http::fields& headers)
    {
        net::ssl::context sslCtx(boost::asio::ssl::context::tlsv13);

        sslCtx.set_verify_mode(net::ssl::verify_peer);
        sslCtx.set_default_verify_paths();

        auto resolver = net::ip::tcp::resolver{ioc};
        net::ssl::stream<beast::tcp_stream> stream(ioc, sslCtx);

        stream.set_verify_callback(
            ssl::host_name_verification(url.host_name()));

        LOG_DEBUG(logger,
                  "DNS look-up of an URL... "
                  "host: {} port:{}",
                  std::string(url.host()), std::string(url.port()));
        auto [ec_resolve, results] = co_await resolver.async_resolve(
            url.host(), url.port() == "" ? "443" : url.port(),
            corral::asio_nothrow_awaitable);

        if (ec_resolve)
            {
                co_return std::unexpected(ec_resolve.message());
            }

        if (!SSL_set_tlsext_host_name(stream.native_handle(),
                                      url.host_name().c_str()))
            {
                co_return std::unexpected(
                    beast::system_error(static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category())
                        .what());
            }

        beast::get_lowest_layer(stream).expires_after(
            std::chrono::seconds(HTTP_MAX_TIME_TIMEOUT_RFC));

        LOG_DEBUG(logger, "Trying to connect to...");
        auto [ec_connect, ep]
            = co_await beast::get_lowest_layer(stream).async_connect(
                results, corral::asio_nothrow_awaitable);
        if (ec_connect)
            {
                co_return std::unexpected(ec_connect.message());
            }
        LOG_DEBUG(logger, "Successfully.");

        LOG_DEBUG(logger, "Trying to do SSL handshake...");
        auto ec_handshake = co_await stream.async_handshake(
            ssl::stream_base::client, corral::asio_nothrow_awaitable);
        if (ec_handshake)
            {
                co_return std::unexpected(ec_handshake.message());
            }
        LOG_DEBUG(logger, "Successfully.");

        LOG_DEBUG(logger,
                  "Creating a request object... "
                  "method: {} path: {} http version:{}",
                  magic_enum::enum_name(method),
                  std::string(url.encoded_resource()), HTTP_VERSION_TO_USE);
        beast::http::request<http::string_body> request{
            method, url.encoded_resource(), HTTP_VERSION_TO_USE, request_body,
            headers};
        request.set(beast::http::field::host, url.host());
        request.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.prepare_payload();
        std::stringstream strs;
        strs << request;
        LOG_TRACE_L1(logger, "Request:\n{}", strs.str());
        beast::get_lowest_layer(stream).expires_after(MAX_PROMPT_TIME);

        LOG_INFO(logger, "Sending request to an LLM...");
        auto [ec_write, bytes_written] = co_await beast::http::async_write(
            stream, request, corral::asio_nothrow_awaitable);
        if (ec_write)
            {
                co_return std::unexpected(ec_write.message());
            }

        beast::flat_buffer buffer(MAX_EXPECTED_CHARACTERS);

        beast::http::response<http::string_body> response;

        LOG_INFO(logger, "Waiting for response...");
        auto [ec_read, bytes_read] = co_await beast::http::async_read(
            stream, buffer, response, corral::asio_nothrow_awaitable);
        if (ec_read)
            {
                co_return std::unexpected(ec_read.message());
            }
        LOG_INFO(logger, "Received response.");

        LOG_DEBUG(logger, "Trying to close connection.");

        auto ec
            = co_await stream.async_shutdown(corral::asio_nothrow_awaitable);

        if (ec && ec != net::ssl::error::stream_truncated)
            {
                co_return std::unexpected(ec.message());
            }

        LOG_DEBUG(logger, "Supposedly closed connection.");

        if (response.result() != http::status::ok)
            {
                co_return std::unexpected("returned with status not 200");
            }

        co_return response.body();
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
                                         ABCCache& cache,
                                         ABCCache& cache_subtitles,
                                         Config const& cfg,
                                         corral::Semaphore& semaphore_yt_dlp,
                                         corral::Semaphore& semaphore_ollama)
    {
        CORRAL_WITH_NURSERY(nursery)
        {
            for (auto& xml_entry : tree.get_child("feed"))
                {
                    if ("entry" != xml_entry.first)
                        {
                            continue;
                        }
                    auto const& link
                        = xml_entry.second.get_child("link.<xmlattr>.href");
                    std::string link_str = link.data();
                    LOG_INFO(logger,
                             "Got link to a YouTube video, maybe... Here's "
                             "the link: {}",
                             link_str);
                    if (not cfg.proceed_with_shorts
                        and link_str.contains("shorts"))
                        {
                            LOG_INFO(logger,
                                     "This is a link to a short. Skipping.");
                            continue;
                        }

                    auto const& author
                        = xml_entry.second.get_child("author.name");
                    auto const& title
                        = xml_entry.second.get_child("media:group.media:title");
                    auto& description = xml_entry.second.get_child(
                        "media:group.media:description");

                    nursery.start(
                        [&, link_str = link_str, author = std::cref(author),
                         title = std::cref(title),
                         description = std::ref(
                             description)]() mutable -> corral::Task<void>
                            {
                                inja::json data;
                                data["author"] = author.get().data();
                                data["title"] = title.get().data();
                                data["description"] = description.get().data();
                                data["link"] = link_str;
                                auto summary_res = co_await summarize(
                                    semaphore_yt_dlp, semaphore_ollama,
                                    link_str, data, ioc, cache, cache_subtitles,
                                    cfg);

                                if (!summary_res)
                                    {
                                        LOG_INFO(logger,
                                                 "Failed to summarize: {}",
                                                 summary_res.error());
                                        co_return;
                                    }

                                LOG_INFO(logger,
                                         "Appending LLM's result to "
                                         "entry's description...");
                                std::string new_description = fmt::format(
                                    "{}\n\nLLM's result:\n{}",
                                    description.get().data(), *summary_res);
                                description.get().put("", new_description);
                                LOG_INFO(logger,
                                         "Successfully appended, I guess...");
                            });
                }
            co_return corral::join;
        };
        LOG_INFO(logger, "Writing result to stdout...");
        std::stringstream strs;
        boost::property_tree::write_xml(strs, tree);
        LOG_INFO(logger, "Wrote result to stdout.");
        co_return strs.str();
    }

    boost::property_tree::ptree parse_rss_into_tree(std::string const& rss_feed)
    {
        LOG_DEBUG(logger, "Received something from stdin...");
        LOG_TRACE_L1(logger, "rss_feed: {}", rss_feed);
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        LOG_DEBUG(logger, "Trying to parse it as an XML...");
        boost::property_tree::read_xml(istr, tree);
        LOG_DEBUG(logger, "Successfully parsed an XML...");
        return tree;
    }

    corral::Task<void> async_main_no_server(auto& ioc, ABCCache& cache,
                                            ABCCache& cache_subtitles,
                                            Config const& cfg)
    {
        LOG_DEBUG(logger, "Waiting for YouTube's RSS feed from stdin...");
        std::cin >> std::noskipws;
        std::istreambuf_iterator<char> start(std::cin);
        std::istreambuf_iterator<char> end;
        std::string xml_rss_youtube_feed(start, end);
        LOG_DEBUG(logger, "Received the YouTube's RSS feed.");

        boost::property_tree::ptree tree
            = parse_rss_into_tree(xml_rss_youtube_feed);

        corral::Semaphore semaphore_yt_dlp(cfg.concurrency_yt_dlp);
        corral::Semaphore semaphore_ollama(cfg.concurrency_ollama);
        std::string res
            = co_await main_logic(ioc, tree, cache, cache_subtitles, cfg,
                                  semaphore_yt_dlp, semaphore_ollama);
        fmt::println("{}", res);
    }

    corral::Task<http::message_generator> handle_request(
        auto& ioc, auto&& req, ABCCache& cache, ABCCache& cache_subtitles,
        Config const& cfg, corral::Semaphore& semaphore_yt_dlp,
        corral::Semaphore& semaphore_ollama)
    {
        auto const bad_request = [&req](beast::string_view why)
            {
                http::response<http::string_body> res{http::status::bad_request,
                                                      req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

        auto const server_error = [&req](beast::string_view what)
            {
                http::response<http::string_body> res{
                    http::status::internal_server_error, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "An error occurred: '" + std::string(what) + "'";
                res.prepare_payload();
                return res;
            };

        if (req.method() != http::verb::get)
            {
                co_return bad_request("Unknown HTTP-method");
            }

        if (req.target().empty() || req.target()[0] != '/'
            || req.target().find("..") != beast::string_view::npos)
            {
                co_return bad_request("Illegal request-target");
            }

        auto res_json = glz::read_json<RequestServer>(req.body());
        if (!res_json)
            {
                co_return bad_request("Request is not correct.");
            }

        const auto& json = res_json.value();

        if (not RE2::FullMatch(
                json.url,
                R"(^https:\/\/www\.youtube\.com\/feeds\/videos\.xml\?channel_id=UC[a-zA-Z0-9_-]{22}$)"))
            {
                co_return bad_request(
                    "Provided URL does not look like an YouTube's URL to an "
                    "RSS "
                    "feed.");
            }

        boost::url url_youtube_rss_feed(json.url);

        auto rss_res = co_await typical_https_request(
            ioc, "", url_youtube_rss_feed, http::verb::get, http::fields{});

        if (!rss_res)
            {
                co_return server_error(rss_res.error());
            }

        std::string response_body = co_await main_logic(
            ioc, parse_rss_into_tree(*rss_res), cache, cache_subtitles, cfg,
            semaphore_yt_dlp, semaphore_ollama);

        http::response<http::string_body> res(http::status::ok, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/xml");
        res.keep_alive(req.keep_alive());
        res.body() = response_body;
        res.prepare_payload();

        co_return res;
    }

}  // namespace

int main(int argc, char* argv[])
{
    Config cfg;

    CLI::App app{
        "Post-processor for YouTube's RSS feed, so that you get "
        "summary of video inside the feed via sending an HTTP "
        "request to something like an Ollama instance."};

    // Temporary storage for CLI11 to map types it doesn't handle natively
    // without custom validators
    std::string url_str = "http://127.0.0.1:11434/api/chat";
    std::string method_str = "post";
    std::vector<std::string> headers_raw = {"Content-Type: application/json"};
    std::string log_level_str = "info";

    app.add_option("-L,--language", cfg.language,
                   "yt-dlp language of subtitles")
        ->capture_default_str()
        ->default_val("en");

    app.add_option(
           "-u,--url", url_str,
           "URL of ?Ollama? instance in format http://127.0.0.1:11434/api/chat")
        ->capture_default_str();

    app.add_option("-X,--method", method_str,
                   "HTTP method by which to ask an ?Ollama? instance. "
                   "Possible values: get, post, head, patch, purge etc.")
        ->capture_default_str();

    app.add_option("-T,--template", cfg.http_body_template,
                   "Jinja template for HTTP request to an ?Ollama? instance.")
        ->default_val(R"({
    "model": "gemma3:4b-it-qat",
    "stream": false,
    "messages": [
      {
        "role": "user",
        "content": "{{ prompt }}"
      }
    ]
})");


    app.add_option("-H,--header", headers_raw,
                   "HTTP headers for request to an ?Ollama? instance.")
        ->take_all();

    app.add_option("-l,--log-file", cfg.log_file, "Filepath to internal logs")
        ->default_val("./logs.log");

    app.add_option(
           "--log-level", log_level_str,
           "Log level: "
           "tracel3,tracel2,tracel1,debug,info,notice,warning,error,critical")
        ->default_val("info");

    app.add_flag("-s,--proceed-shorts", cfg.proceed_with_shorts,
                 "Try do with shorts");

    app.add_flag("-A,--enable-server", cfg.enable_server, "Enable server")
        ->default_val(false);
    app.add_flag("-p,--port", cfg.server_port, "Server's port")
        ->check(CLI::PositiveNumber)
        ->default_val(SERVER_DEFAULT_PORT);

    app.add_option(
           "-j,--jobs-yt-tlp", cfg.concurrency_yt_dlp,
           "Amount of concurrent yt-dlp processes created by this application.")
        ->check(CLI::PositiveNumber)
        ->default_val(MAX_CONCURRENT_YTDLP_DEFAULT);

    app.add_option("-J,--jobs-requests", cfg.concurrency_ollama,
                   "Amount of concurrent request to an ?Ollama? instance sent "
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
                    boost::split(parts, header_raw_str, boost::is_any_of(":"));
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
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nThis is an Omega exception. Most definitely an incorrect "
                "value was specified in args. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to get "
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
                trace = boost::stacktrace::stacktrace::from_current_exception();
            fmt::print(
                std::cerr,
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nYou know that standard exceptions sucks. Here's "
                ".what():\n\n{}\n\nHere's an attempt to get backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace... Hope it works or kill "
                "youself debugging this shit.\n{}\n",
                e.what(), boost::stacktrace::to_string(trace));
            return std::to_underlying(
                ReturnCodes::FailDuringInitializationConfig);
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
            logger = quill::Frontend::create_or_get_logger(
                "root", std::move(file_sink));
            logger->set_log_level(cfg.log_level);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            fmt::print(
                std::cerr,
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nYou know that standard exceptions sucks. Here's "
                ".what():\n\n{}\n\nHere's an attempt to get backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace... Hope it works or kill "
                "youself debugging this shit.\n{}\n",
                e.what(), boost::stacktrace::to_string(trace));
            return std::to_underlying(ReturnCodes::FailInitializationLogger);
        }

    try
        {
            quill::BackendOptions backend_options;
            backend_options.check_printable_char = {};
            quill::Backend::start(backend_options);

            LOG_DEBUG(logger, "Successfully parsed command line arguments.");

            CacheHexHashFile cache(cfg.cache_file);
            LOG_DEBUG(logger, "Successfully created cache object.");
            CacheHexHashFile cache_subtitles(cfg.cache_subtitles_file);
            LOG_DEBUG(logger,
                      "Successfully created cache object for subtitles.");

            LOG_INFO(logger, "Trying to parse supplied headers...");

            net::io_context ioc;
            LOG_DEBUG(logger, "Entering coroutine...");
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            corral::run(
                ioc, corral::anyOf(
                         async_main_no_server(ioc, cache, cache_subtitles, cfg),
                         signals.async_wait(corral::asio_awaitable)));
        }
    catch (OmegaException<std::filesystem::path>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nThis is an Omega exception. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to get "
                "backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace...",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR(logger, "{}", log);
            return std::to_underlying(ReturnCodes::FailCacheFolder);
        }
    catch (OmegaException<std::string>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nThis is an Omega exception. Maybe from parsing result from "
                "Ollama. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to get "
                "backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace...",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR(logger, "{}", log);
            return std::to_underlying(ReturnCodes::FailParsePromptResult);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely guy. "
                "\nYou know that standard exceptions sucks. Here's "
                ".what():\n\n{}\n\nHere's an attempt to get backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace... Hope it works or kill "
                "youself debugging this shit.\n{}\n",
                e.what(), boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR(logger, "{}", log);
            return std::to_underlying(ReturnCodes::FailStandardException);
        }
    return std::to_underlying(ReturnCodes::Success);
}
