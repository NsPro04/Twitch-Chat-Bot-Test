#include <nlohmann/json.hpp>

#include "session_welcome/session_welcome.h"

#include <format>

// https://dev.twitch.tv/docs/eventsub/handling-websocket-events#welcome-message
std::string session_welcome(std::unique_ptr<httplib::ws::WebSocketClient> &ws) {
  std::string msg_str;

  ws->read(msg_str);

  const nlohmann::json msg = nlohmann::json::parse(std::move(msg_str));

  if (msg["metadata"]["message_type"] != "session_welcome" || msg["payload"]["session"]["id"].type() != nlohmann::json::value_t::string)
    throw std::runtime_error(std::format("Expected session_welcome message with string session id, got: {}", msg.dump(2)));

  return msg["payload"]["session"]["id"].get<std::string>();
};