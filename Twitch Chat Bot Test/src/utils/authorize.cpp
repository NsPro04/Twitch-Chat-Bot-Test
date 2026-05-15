#include "utils/authorize.h"

#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/task_group.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "utils/content_types.h"
#include "utils/new_srv.h"
#include "utils/get_current_time.h"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <format>
#include <regex>

using njson = nlohmann::json;

constexpr size_t           REDIRECT_PORT = 12345;
constexpr std::string_view REDIRECT_HOST = "127.0.0.1";

constexpr std::string_view REDIRECT_PROTOCOL = "http://";
constexpr std::string_view REDIRECT_DOMAIN = "localhost";
constexpr std::string_view REDIRECT_PATH = "/dummy";

constexpr std::string_view REDIRECT2_PATH = "/response";

std::string authorize(std::string_view CLIENT_ID) {
  [[maybe_unused]] tbb::global_control _global_control(tbb::global_control::max_allowed_parallelism, (std::max<size_t>)(4, tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)));

  std::string access_token;

  {
    tbb::task_group tasks;
    std::unique_ptr<httplib::Server> srv_ptr = new_srv();

    std::atomic<bool> access_token_received = false;

    std::once_flag stop_called_flag;

    srv_ptr->set_exception_handler([&]([[maybe_unused]] const httplib::Request &req, httplib::Response &res, std::exception_ptr ep) {
      std::call_once(stop_called_flag, [&] { srv_ptr->stop(); });

      try {
        if (ep) std::rethrow_exception(ep);
      } catch (const std::exception &e) {
        spdlog::error("Server exception: {}", e.what());
        res.set_content(njson{{"error", e.what()}}.dump(2), std::string(CONTENT_TYPE_JSON));
      } catch (...) {
        spdlog::error("Unknown server exception.");
        res.set_content(njson{{"error", "Unknown error"}}.dump(2), std::string(CONTENT_TYPE_JSON));
      }
    });

    // dirty hack
    srv_ptr->Get(std::string(REDIRECT_PATH), []([[maybe_unused]] const httplib::Request &req, httplib::Response &res) {
      res.set_content(std::format(R"(
        <html>
          <head><title>{}</title></head>
          <body>
            <script>
              window.location.href = `{}{}:{}{}?url=${{encodeURIComponent(window.location.href)}}`;
            </script>
          </body>
        </html>
      )", REDIRECT_PATH, REDIRECT_PROTOCOL, REDIRECT_DOMAIN, REDIRECT_PORT, REDIRECT2_PATH), std::string(CONTENT_TYPE_HTML));
    });

    srv_ptr->Get(std::string(REDIRECT2_PATH), [&](const httplib::Request &req, httplib::Response &res) {
      std::call_once(stop_called_flag, [&] { srv_ptr->stop(); });

      if (!req.has_param("url"))
        throw std::runtime_error("Failed to get OAuth token: No \"url\" parameter in request.");

      std::string url = req.get_param_value("url");

      std::smatch match;

      if (!std::regex_search(url, match, std::regex("access_token=(\\w+)")) || match.size() != 2)
        throw std::runtime_error(std::format("Failed to get OAuth token: No access_token found in URL parameter. url={}", url));

      if (access_token_received.exchange(true))
        return;

      try {
        access_token = match.str(1);
      } catch (...) {
        access_token_received.exchange(false);

        throw std::runtime_error("Failed to save access_token.");
      }

      res.set_content(njson{
        {"msg", "Кажется авторизация прошла успешно 🤔"},
        {"current_time", get_current_time()}
      }.dump(2), std::string(CONTENT_TYPE_JSON));
    });

    tasks.run([&] {
      if (!srv_ptr->listen(std::string(REDIRECT_HOST), REDIRECT_PORT)) {
        throw std::runtime_error(std::format("Failed to start server on {}:{}", REDIRECT_HOST, REDIRECT_PORT));
      }
    });

    srv_ptr->wait_until_ready();

    // https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#implicit-grant-flow
    std::system(std::format("start \"\" \"https://id.twitch.tv/oauth2/authorize?{}&{}&{}&{}\"",
      std::format("response_type={}", "token"),
      std::format("client_id={}",    CLIENT_ID),
      std::format("redirect_uri={}", httplib::encode_uri_component(std::format("{}{}:{}{}", REDIRECT_PROTOCOL, REDIRECT_DOMAIN, REDIRECT_PORT, REDIRECT_PATH))),
      std::format("scope={}",        httplib::encode_uri_component("user:bot user:read:chat user:write:chat"))
    ).c_str());

    tasks.wait();

    if (!access_token_received.load())
      throw std::runtime_error("Failed to get OAuth token: Access token was not received.");
  }

  return access_token;
};