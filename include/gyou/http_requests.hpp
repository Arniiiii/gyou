#ifndef INCLUDE_GYOU_HTTP_REQUESTS_HPP_
#define INCLUDE_GYOU_HTTP_REQUESTS_HPP_

#include <expected>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/url.hpp>
#include <boost/url/urls.hpp>
#include <corral/Task.h>

constexpr inline size_t MAX_EXPECTED_CHARACTERS = 128000;
constexpr inline auto MAX_PROMPT_TIME = std::chrono::minutes(10);
constexpr inline auto HTTP_MAX_TIME_TIMEOUT_RFC = std::chrono::seconds(120);
constexpr inline int HTTP_VERSION_TO_USE = 11;

corral::Task<std::expected<std::string, std::string>> typical_http_request(
    boost::asio::io_context& ioc, std::string const& request_body,
    const boost::url& url, boost::beast::http::verb method,
    boost::beast::http::fields headers);

corral::Task<std::expected<std::string, std::string>> typical_https_request(
    boost::asio::io_context& ioc, std::string const& request_body,
    boost::url const& url, boost::beast::http::verb method,
    const boost::beast::http::fields& headers);

corral::Task<std::expected<std::string, std::string>> request_internet(
    boost::asio::io_context& ioc, std::string const& request_body,
    boost::urls::url const& url, boost::beast::http::verb method,
    boost::beast::http::header<true> const& headers);

#endif  // INCLUDE_GYOU_HTTP_REQUESTS_HPP_
