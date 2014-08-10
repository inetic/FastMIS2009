#include <boost/uuid/uuid_io.hpp>
#include <boost/random/random_device.hpp>

#include "Node.h"
#include "Connection.h"
#include "constants.h"
#include "protocol.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
using ErrorCode = boost::system::error_code;

Node::Node(boost::asio::io_service& ios)
  : _io_service(ios)
  , _socket(ios, udp::endpoint(udp::v4(), 0)) // Assign random port
{
}

void Node::start() {
  receive_data();
}

void Node::shutdown() {
  _socket.close();
  _connections.clear();
}

void Node::receive_data() {
  struct ServerData {
    std::vector<char> data;
    udp::endpoint     sender_endpoint;

    ServerData() : data(MAX_DATAGRAM_SIZE) {}
  };

  auto server_data = make_shared<ServerData>();

  _socket.async_receive_from
    ( asio::buffer(server_data->data)
    , server_data->sender_endpoint
    , [this, server_data](const ErrorCode& ec, std::size_t size) {
        if (ec) {
          if (ec != asio::error::operation_aborted) {
            receive_data();
          }
          return;
        }

        use_data( server_data->sender_endpoint
                , string( server_data->data.begin()
                        , server_data->data.begin() + size));

        receive_data();
      });
}

void Node::use_data(Endpoint sender, string&& data) {
  auto& c = create_connection(sender);

  try {
    c.receive_data(data);
  }
  catch (const runtime_error& e) {
    cout << "Problem reading message: " << e.what() << endl;
    disconnect(sender);
  }
}

Connection& Node::create_connection(Endpoint endpoint) {
  auto c_i = _connections.find(endpoint);

  if (c_i != _connections.end()) {
    return *c_i->second;
  }

  auto c = make_shared<Connection>(*this, endpoint);
  _connections[endpoint] = c;

  return *c;
}

void Node::connect(Endpoint remote_endpoint) {
  create_connection(remote_endpoint);
}

void Node::disconnect(Endpoint remote_endpoint) {
  _connections.erase(remote_endpoint);
}

bool Node::is_connected_to(Endpoint remote_endpoint) const {
  return _connections.count(ID(remote_endpoint)) != 0;
}

void Node::send_to(const Message& msg, Endpoint destination) {
  stringstream ss;
  ss << msg.label() << " ";
  msg.to_stream(ss);

  auto data = make_shared<string>(ss.str());
  _socket.async_send_to( asio::buffer(*data)
                       , destination
                       , [data](boost::system::error_code, size_t) {});
}

static float random_number() {
  typedef boost::random_device Dev;
  Dev generate;
  Dev::result_type random_number = generate();
  return (float) (random_number - Dev::min()) / Dev::max();
}

bool Node::received_random_number_from_all_peers() const {
  for (const auto& pair : _connections) {
    const auto& c = *pair.second;
    if (!c.random_number) {
      return false;
    }
  }
  return true;
}

bool Node::smaller_than_others(float my_number) const {
  for (const auto& pair : _connections) {
    const auto& c = *pair.second;
    if (c.random_number && my_number >= c.random_number) {
      return false;
    }
  }
  return true;
}

void Node::start_fast_mis() {
  cout << id() << " start_fast_mis\n";

  if (!_my_random_number) {
    cout << id() << " start_fast_mis choosing random number\n";
    _my_random_number = random_number();

    for (const auto& pair : _connections) {
      auto& c = *pair.second;
      if (c.knows_my_number) { continue; }
      c.knows_my_number = true;
      c.start_fast_mis(*_my_random_number);
    }
  }

  if (!received_random_number_from_all_peers()) {
    return;
  }

  if (smaller_than_others(*_my_random_number)) {
    cout << id() << " Leader" << endl;
  }
  else {
    cout << id() << " Not leader" << endl;
  }
}

// So that I can use std::unique_ptr with forward declared template
// parameter in Node definition.
Node::~Node() {}

