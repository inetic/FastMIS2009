#include <iostream>
#include "Connection.h"
#include "Node.h"
#include "protocol.h"
#include "constants.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
namespace pstime = boost::posix_time;
using Error = boost::system::error_code;


Connection::Connection(Node& node, Endpoint remote_endpoint)
  : _node(node)
  , _remote_endpoint(remote_endpoint)
  , _periodic_timer(node.get_io_service(), [=]() { on_tick(); })
  , _missed_ping_count(0)
{
}

void Connection::on_tick() {
  if (_missed_ping_count > MAX_MISSED_PING_COUNT) {
    // This might destroy this object, so make sure you
    // return immediately.
    _node.disconnect(_remote_endpoint);
    return;
  }
  ++_missed_ping_count;
  _node.send_to(Ping(), _remote_endpoint);
}

void Connection::keep_alive() {
  _missed_ping_count = 0;
}

