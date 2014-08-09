#include <iostream>
#include "Connection.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;

Connection::Connection(Endpoint remote_endpoint)
  : _remote_endpoint(remote_endpoint)
{
}

