#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <memory>
#include <string_view>

std::unique_ptr<httplib::ws::WebSocketClient> new_wscli(std::string_view path);