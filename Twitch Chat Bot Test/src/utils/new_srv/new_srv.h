#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <memory>

std::unique_ptr<httplib::Server> new_srv();