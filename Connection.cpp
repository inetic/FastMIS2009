#include <iostream>
#include "Connection.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;

Connection::Connection(asio::io_service& ios)
  : _io_service(ios)
  , _socket(ios, udp::endpoint(udp::v4(), 0)) // Assign random port
{
  cout << _socket.local_endpoint().port() << endl;
}

void Connection::start(const asio::ip::udp::endpoint& remote_endpoint) {
}

