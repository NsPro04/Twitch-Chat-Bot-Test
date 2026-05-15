#include "utils/new_srv/new_srv.h"

static std::unique_ptr<httplib::Server> new_srv() {
  auto srv_ptr = std::make_unique<httplib::Server>();

  srv_ptr->set_keep_alive_max_count(1);
  srv_ptr->set_keep_alive_timeout(60);
  srv_ptr->set_read_timeout(60);
  srv_ptr->set_write_timeout(60);

  return srv_ptr;
};