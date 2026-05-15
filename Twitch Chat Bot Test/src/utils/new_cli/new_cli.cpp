#include "utils/new_cli/new_cli.h"

#include <string>

static std::unique_ptr<httplib::Client> new_cli(std::string_view path) {
  auto cli_ptr = std::make_unique<httplib::Client>(std::string(path));

  cli_ptr->enable_server_certificate_verification(false);
  cli_ptr->enable_server_hostname_verification(false);
  cli_ptr->set_connection_timeout(60);
  cli_ptr->set_read_timeout(60);
  cli_ptr->set_write_timeout(60);
  cli_ptr->set_keep_alive(false);

  return cli_ptr;
};