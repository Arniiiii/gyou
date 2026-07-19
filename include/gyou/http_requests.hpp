#ifndef INCLUDE_GYOU_HTTP_REQUESTS_HPP_
#define INCLUDE_GYOU_HTTP_REQUESTS_HPP_

#include <expected>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>
#include <magic_enum/magic_enum.hpp>

#include "overwrite_log_macros.h"

constexpr size_t MAX_EXPECTED_CHARACTERS = 128000;
constexpr auto MAX_PROMPT_TIME = std::chrono::minutes(10);
constexpr auto HTTP_MAX_TIME_TIMEOUT_RFC = std::chrono::seconds(120);
constexpr int HTTP_VERSION_TO_USE = 11;

corral::Task<std::expected<std::string, std::string>> typical_http_request(
    auto& ioc, std::string const& request_body, const boost::url& url,
    boost::beast::http::verb method, boost::beast::http::fields headers)
{
    auto resolver = boost::asio::ip::tcp::resolver{ioc};

    LOG_DEBUG(
        "DNS look-up of an URL... "
        "host: {} port:{}",
        std::string(url.host()), std::string(url.port()));
    auto [ec_resolve, results] = co_await resolver.async_resolve(
        url.host(), url.port(), corral::asio_nothrow_awaitable);
    if (ec_resolve)
        {
            co_return std::unexpected(ec_resolve.message());
        }

    auto stream = boost::beast::tcp_stream{ioc};
    stream.expires_after(std::chrono::seconds(HTTP_MAX_TIME_TIMEOUT_RFC));

    LOG_DEBUG("Trying to connect to an URL...");
    auto [ec_connect, ep] = co_await stream.async_connect(
        results, corral::asio_nothrow_awaitable);
    if (ec_connect)
        {
            co_return std::unexpected(ec_connect.message());
        }
    LOG_DEBUG("Successfully.");

    LOG_DEBUG(
        "Creating a request object... "
        "method: {} path: {} http version:{}",
        magic_enum::enum_name(method), std::string(url.encoded_resource()),
        HTTP_VERSION_TO_USE);
    boost::beast::http::request<boost::beast::http::string_body> request{
        method, url.encoded_resource(), HTTP_VERSION_TO_USE, request_body,
        headers};
    request.set(boost::beast::http::field::host, url.host());
    request.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
    request.prepare_payload();
    std::stringstream strs;
    strs << request;
    LOG_TRACE_L1("Request:\n{}", strs.str());
    stream.expires_after(MAX_PROMPT_TIME);

    LOG_INFO("Sending request to an LLM...");
    auto [ec_write, bytes_written] = co_await boost::beast::http::async_write(
        stream, request, corral::asio_nothrow_awaitable);
    if (ec_write)
        {
            co_return std::unexpected(ec_write.message());
        }

    boost::beast::flat_buffer buffer(MAX_EXPECTED_CHARACTERS);

    boost::beast::http::response<boost::beast::http::string_body> response;

    LOG_INFO("Waiting for response...");
    auto [ec_read, bytes_read] = co_await boost::beast::http::async_read(
        stream, buffer, response, corral::asio_nothrow_awaitable);
    if (ec_read)
        {
            co_return std::unexpected(ec_read.message());
        }
    LOG_INFO("Received response.");

    LOG_DEBUG("Trying to close connection.");

    boost::beast::error_code error_code;
    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                             error_code);

    if (error_code && error_code != boost::beast::errc::not_connected)
        {
            co_return std::unexpected(error_code.message());
        }
    LOG_DEBUG("Supposedly closed connection.");

    if (response.result() != boost::beast::http::status::ok)
        {
            co_return std::unexpected("returned with status not 200");
        }

    co_return response.body();
}

corral::Task<std::expected<std::string, std::string>> typical_https_request(
    auto& ioc, std::string const& request_body, boost::url const& url,
    boost::beast::http::verb method, const boost::beast::http::fields& headers)
{
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tlsv13);

    sslCtx.set_verify_mode(boost::asio::ssl::verify_peer);
    sslCtx.set_default_verify_paths();

    auto resolver = boost::asio::ip::tcp::resolver{ioc};
    boost::asio::ssl::stream<boost::beast::tcp_stream> stream(ioc, sslCtx);

    stream.set_verify_callback(
        boost::asio::ssl::host_name_verification(url.host_name()));

    LOG_DEBUG(
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

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    if (!SSL_set_tlsext_host_name(stream.native_handle(),
                                  url.host_name().c_str()))
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
        {
            co_return std::unexpected(
                boost::beast::system_error(
                    static_cast<int>(::ERR_get_error()),
                    boost::asio::error::get_ssl_category())
                    .what());
        }

    boost::beast::get_lowest_layer(stream).expires_after(
        std::chrono::seconds(HTTP_MAX_TIME_TIMEOUT_RFC));

    LOG_DEBUG("Trying to connect to...");
    auto [ec_connect, ep]
        = co_await boost::beast::get_lowest_layer(stream).async_connect(
            results, corral::asio_nothrow_awaitable);
    if (ec_connect)
        {
            co_return std::unexpected(ec_connect.message());
        }
    LOG_DEBUG("Successfully.");

    LOG_DEBUG("Trying to do SSL handshake...");
    auto ec_handshake = co_await stream.async_handshake(
        boost::asio::ssl::stream_base::client, corral::asio_nothrow_awaitable);
    if (ec_handshake)
        {
            co_return std::unexpected(ec_handshake.message());
        }
    LOG_DEBUG("Successfully.");

    LOG_DEBUG(
        "Creating a request object... "
        "method: {} path: {} http version:{}",
        magic_enum::enum_name(method), std::string(url.encoded_resource()),
        HTTP_VERSION_TO_USE);
    boost::beast::http::request<boost::beast::http::string_body> request{
        method, url.encoded_resource(), HTTP_VERSION_TO_USE, request_body,
        headers};
    request.set(boost::beast::http::field::host, url.host());
    request.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
    request.prepare_payload();
    std::stringstream strs;
    strs << request;
    LOG_TRACE_L2("Request:\n{}", strs.str());
    boost::beast::get_lowest_layer(stream).expires_after(MAX_PROMPT_TIME);

    LOG_INFO("Sending request to an LLM...");
    auto [ec_write, bytes_written] = co_await boost::beast::http::async_write(
        stream, request, corral::asio_nothrow_awaitable);
    if (ec_write)
        {
            co_return std::unexpected(ec_write.message());
        }

    boost::beast::flat_buffer buffer(MAX_EXPECTED_CHARACTERS);

    boost::beast::http::response<boost::beast::http::string_body> response;

    LOG_INFO("Waiting for response...");
    auto [ec_read, bytes_read] = co_await boost::beast::http::async_read(
        stream, buffer, response, corral::asio_nothrow_awaitable);
    if (ec_read)
        {
            co_return std::unexpected(ec_read.message());
        }
    LOG_INFO("Received response.");

    LOG_DEBUG("Trying to close connection.");

    auto errc = co_await stream.async_shutdown(corral::asio_nothrow_awaitable);

    if (errc && errc != boost::asio::ssl::error::stream_truncated)
        {
            co_return std::unexpected(errc.message());
        }

    LOG_DEBUG("Supposedly closed connection.");

    if (response.result() != boost::beast::http::status::ok)
        {
            co_return std::unexpected("returned with status not 200");
        }

    co_return response.body();
}

corral::Task<std::expected<std::string, std::string>> request_internet(
    auto& ioc, std::string const& request_body, boost::urls::url const& url,
    boost::beast::http::verb method,
    boost::beast::http::header<true> const& headers)
{
    if ("https" == url.scheme())
        {
            co_return co_await typical_https_request(ioc, request_body, url,
                                                     method, headers);
        }
    else
        {
            co_return co_await typical_http_request(ioc, request_body, url,
                                                    method, headers);
        }
}

#endif  // INCLUDE_GYOU_HTTP_REQUESTS_HPP_
