#include <iostream>
#include "Connection.h"
#include "Node.h"
#include "protocol.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
namespace pstime = boost::posix_time;
using Error = boost::system::error_code;

Connection::Connection(Node& node, Endpoint remote_endpoint)
  : _node(node)
  , _remote_endpoint(remote_endpoint)
  , _periodic_timer(node.get_io_service(), [=]() { on_tick(); })
{
}

void Connection::on_tick() {
  _node.send_to(Ping(), _remote_endpoint);
}
