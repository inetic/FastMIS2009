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
  , _id(_socket.local_endpoint())
  , _was_shut_down(false)
{
  receive_data();
}

void Node::shutdown() {
  cout << id() << " shutdown\n";
  _was_shut_down = true;
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
        if (_was_shut_down) return;

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

template<class Message, class... Args> void Node::broadcast(Args... args) {
  for (auto c_id : _contenders) {
    auto& c = *_connections[c_id];
    c.schedule_send<Message>(args...);
  }
}

template<class ContenderIDs, class Connections, class F>
void for_each_contender(const ContenderIDs& ids, Connections& cs, const F& f) {
  ContenderIDs copy = ids;
  for (auto id : copy) {
    auto ci = cs.find(id);
    assert(ci != cs.end());
    f(*ci->second);
  }
}

template<class F> void Node::each_contender(const F& f) const {
  for_each_contender(_contenders, _connections, f);
}

template<class F> void Node::each_contender(const F& f) {
  for_each_contender(_contenders, _connections, f);
}

static float random_number() {
  typedef boost::random_device Dev;
  Dev generate;
  Dev::result_type random_number = generate();
  return (float) (random_number - Dev::min()) / Dev::max();
}

bool Node::has_number_from_all() const {
  bool retval = true;
  each_contender([&](const Connection& c) {
      if (!c.random_number) retval = false;
      });
  return retval;
}

bool Node::has_status_from_all() const {
  bool retval = true;
  each_contender([&](const Connection& c) {
      if (!c.leader_status) retval = false;
      });
  return retval;
}

bool Node::smaller_than_others(float my_number) const {
  bool retval = true;
  each_contender([&](const Connection& c) {
      if (c.random_number && my_number >= c.random_number) {
        retval = false;
      }});
  return retval;
}

void Node::on_algorithm_completed() {
  _fast_mis_started = false;
  cout << id() << " !!! " << _leader_status << " !!!\n";
  if (_on_algorithm_completed) {
    auto handler = _on_algorithm_completed;
    handler();
  }
}

bool Node::has_leader_neighbor() const {
  for (const auto& pair : _connections) {
    const auto& c = *pair.second;
    if (c.leader_status && *c.leader_status == LeaderStatus::leader) {
      return true;
    }
  }
  return false;
}

void Node::start_fast_mis() {
  _contenders.clear();
  for (const auto& pair : _connections) {
    _contenders.insert(pair.first);
  }

  _leader_status = LeaderStatus::undecided;
  _fast_mis_started = true;
  cout << id() << " scheduling start broadcast\n";
  broadcast<StartMsg>();
  on_receive_number();
  cout << id() << " start_fast_mis\n";
}

void Node::on_received_start() {
  if (_fast_mis_started) return;
  _fast_mis_started = true;
  start_fast_mis();
}

void Node::on_receive_number() {
  if (!_my_random_number) {
    _my_random_number = random_number();
    cout << id() << " broadcasting new number " << *_my_random_number << "\n";
    broadcast<NumberMsg>(*_my_random_number);
  }

  if (!has_number_from_all()) {
    cout << id() << " on_received_number: don't have all numbers\n";
    return;
  }

  cout << id() << " on_received_number: have all numbers\n";

  if (smaller_than_others(*_my_random_number)) {
    _leader_status = LeaderStatus::leader;
  }

  // We don't these anymore and they need to be unsed for the next stage.
  _my_random_number.reset();
  each_contender([](Connection& c) { c.random_number.reset(); });

  broadcast<StatusMsg>(_leader_status);

  on_receive_status();
}

void Node::on_receive_status() {
  if (!has_status_from_all()) return;

  cout << id() << " on_receive_status" << endl;

  if (has_leader_neighbor()) {
    _leader_status = LeaderStatus::follower;
  }

  each_contender([&](Connection& c) {
      if (*c.leader_status != LeaderStatus::undecided) {
        _contenders.erase(c.id());
      }
      else {
        c.leader_status.reset();
      }
      });

  if (_leader_status == LeaderStatus::undecided) {
    on_receive_number();
  }
  else {
    broadcast<ResultMsg>(_leader_status);
    on_algorithm_completed();
  }
}

void Node::on_receive_result(Connection& from) {
  _contenders.erase(from.id());
  on_receive_number();
}

// So that I can use std::unique_ptr with forward declared template
// parameter in Node definition.
Node::~Node() {}

std::ostream& operator<<(std::ostream& os, const Node& node) {
  os << node.id() << ": ";
  for (const auto& pair : node._connections) {
    os << pair.second->id() << " ";
  }
  return os;
}
