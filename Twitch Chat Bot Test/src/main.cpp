#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "utils/content_types.h"
#include "authorize/authorize.h"
#include "utils/new_wscli/new_wscli.h"
#include "session_welcome/session_welcome.h"
#include "utils/new_cli/new_cli.h"

#include <string_view>
#include <chrono>
#include <string>
#include <memory>
#include <stdexcept>
#include <format>
#include <thread>
#include <regex>

constexpr std::string_view CLIENT_ID = "8tcrk8hcpedfk2byde809qf946nz60";


[[maybe_unused]] constexpr std::string_view GRUZCHIK_TASHIT_ID = "29916300";
[[maybe_unused]] constexpr std::string_view NSPRO123_ID = "467322333";

constexpr std::string_view BROADCASTER_ID = NSPRO123_ID;
constexpr std::string_view USER_ID = NSPRO123_ID;

constexpr std::string_view TWITCH_WEBSOCKET_URL = "wss://eventsub.wss.twitch.tv/ws";

constexpr std::string_view TWITCH_API_BASE_URL = "https://api.twitch.tv";
constexpr std::string_view TWITCH_SUBSCRIPTION_URL = "/helix/eventsub/subscriptions";
constexpr std::string_view TWITCH_CHAT_MESSAGES_URL = "/helix/chat/messages";

using namespace std::chrono_literals;
using njson = nlohmann::json;

static void _main() {
  std::string access_token = authorize(CLIENT_ID);

  spdlog::info("Access token successfully obtained.");

  std::unique_ptr<httplib::ws::WebSocketClient> ws = new_wscli(TWITCH_WEBSOCKET_URL);

  std::string session_id = session_welcome(ws);

  spdlog::info("WebSocket session established successfully.");

  { // Create Twitch EventSub subscription
    std::unique_ptr<httplib::Client> cli = new_cli(TWITCH_API_BASE_URL);

    // https://dev.twitch.tv/docs/api/reference#create-eventsub-subscription
    httplib::Result result = cli->Post(std::string(TWITCH_SUBSCRIPTION_URL), {
      {"Authorization", std::format("Bearer {}", access_token)},
      {"Client-Id",     std::format("{}",        CLIENT_ID)}
    }, njson({
      {"type",      "channel.chat.message"},
      {"version",   "1"},
      {"condition", njson({{"broadcaster_user_id", BROADCASTER_ID}, {"user_id", USER_ID}})},
      {"transport", njson({{"method", "websocket"}, {"session_id", session_id}})}
    }).dump(), std::string(CONTENT_TYPE_JSON));

    if (!result)
      throw std::runtime_error(std::format("Failed to create Twitch EventSub subscription: {}", httplib::to_string(result.error())));

    if (result->status != 202)
      throw std::runtime_error(std::format("Failed to create Twitch EventSub subscription: {} {}", result->status, result->body));
  }

  spdlog::info("Twitch EventSub subscription created successfully.");

  std::string msg_str;

  while (ws->read(msg_str)) {
    njson msg = njson::parse(msg_str);

    std::string message_type = msg["metadata"]["message_type"].get<std::string>();

    if (message_type == "session_keepalive") {
      spdlog::info("session_keepalive");
      continue;
    };

    if (message_type != "notification") {
      spdlog::error("(message_type != \"notification\"): {}", msg.dump());
      break;
    }

    std::string message_id = msg["payload"]["event"]["message_id"].get<std::string>();

    std::string user_name = msg["payload"]["event"]["chatter_user_name"].get<std::string>();
    std::string user_login = msg["payload"]["event"]["chatter_user_login"].get<std::string>();

    std::string msg_text = msg["payload"]["event"]["message"]["text"].get<std::string>();

    spdlog::info("{}({}): '{}' {}", user_name, user_login, msg_text, msg_text.size());

    if (std::regex_search(msg_text, std::regex("(?:(?:^![Pp][Ii][Nn][Gg])|(?:^!(?:П|п)(?:И|и)(?:Н|н)(?:Г|г)))"))) {
      auto current_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

      std::unique_ptr<httplib::Client> cli = new_cli(TWITCH_API_BASE_URL);

      // https://dev.twitch.tv/docs/api/reference#send-chat-message
      httplib::Result result = cli->Post(std::string(TWITCH_CHAT_MESSAGES_URL), {
        {"Authorization", std::format("Bearer {}", access_token)},
        {"Client-Id",     std::format("{}",        CLIENT_ID)}
      }, njson({
        {"broadcaster_id", BROADCASTER_ID},
        {"sender_id", USER_ID},
        {"message", std::format("pong [{0:%F} {0:%T}]", current_time)},
        {"reply_parent_message_id", message_id}
      }).dump(), std::string(CONTENT_TYPE_JSON));
    }
  }

  spdlog::info("END");
};

int wmain() try {
  SetConsoleOutputCP(CP_UTF8);

  _main();

  return 0;
} catch (const std::exception &e) {
  spdlog::error("Exception: {}", e.what());
  std::this_thread::sleep_for(2s);
  return 1;
} catch (...) {
  spdlog::error("Something went wrong.");
  std::this_thread::sleep_for(2s);
  return 1;
};