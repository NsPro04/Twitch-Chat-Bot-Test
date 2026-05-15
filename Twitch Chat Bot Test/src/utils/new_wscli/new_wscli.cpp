#include "utils/new_wscli/new_wscli.h"

#include <string>
#include <stdexcept>

static std::unique_ptr<httplib::ws::WebSocketClient> new_wscli(std::string_view path) {
  auto wscli_ptr = std::make_unique<httplib::ws::WebSocketClient>(std::string(path));

  wscli_ptr->enable_server_certificate_verification(false);

  wscli_ptr->set_connection_timeout(60);
  wscli_ptr->set_read_timeout(60);
  wscli_ptr->set_write_timeout(60);

  if (!wscli_ptr->connect())
    throw std::runtime_error("Failed to connect to Twitch EventSub WebSocket.");

  return wscli_ptr;
};