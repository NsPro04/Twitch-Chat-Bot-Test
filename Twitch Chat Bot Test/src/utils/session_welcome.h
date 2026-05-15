#pragma once

#include "utils/new_wscli.h"

#include <string>

std::string session_welcome(std::unique_ptr<httplib::ws::WebSocketClient> &ws);