#include <boost/uuid/uuid_io.hpp>

#include "Node.h"
#include "Connection.h"
#include "Random.h"
#include "constants.h"
#include "protocol.h"
#include "log.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
using ErrorCode = boost::system::error_code;

Node::Node(boost::asio::io_service& ios)
  : _io_service(ios)
  , _socket(ios, udp::endpoint(udp::v4(), 0)) // Assign random port
  , _id(_socket.local_endpoint())
  , _was_shut_down(false)
  , _ping_timeout(boost::posix_time::milliseconds(PING_TIMEOUT_MS))
  , _max_missed_ping_count(MAX_MISSED_PING_COUNT)
  , _state(idle)
{
  receive_data();
}

void Node::shutdown() {
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
  auto destroyed   = _destroy_guard.indicator();

  _socket.async_receive_from
    ( asio::buffer(server_data->data)
    , server_data->sender_endpoint
    , [this, server_data, destroyed](const ErrorCode& ec, std::size_t size) {
        if (destroyed) return;
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
  try {
    std::stringstream ss(data);
    dispatch_message(ss
        , [&](const PingMsg& msg)    { use_data(sender, msg); }
        , [&](const StartMsg& msg)   { use_data(sender, msg); }
        , [&](const NumberMsg& msg)  { use_data(sender, msg); }
        , [&](const Update1Msg& msg) { use_data(sender, msg); }
        , [&](const Update2Msg& msg) { use_data(sender, msg); }
        , [&](const ResultMsg& msg)  { use_data(sender, msg); });
  }
  catch (const runtime_error& e) {
    log(id(), " Problem reading message: ", e.what());
    connection_lost(sender);
  }
}

template<class Msg>
void Node::use_data(Endpoint sender, const Msg& msg) {
  auto c_i = _connections.find(sender);

  if (c_i == _connections.end()) {
    // Only the first message can be used to establish connection.
    if (msg.sequence_number != 1) {
      return;
    }
    c_i = create_connection(sender);
  }

  auto& c = *c_i->second;
  c.receive(msg);
}

Node::Connections::iterator Node::create_connection(Endpoint endpoint) {
  auto c = unique_ptr<Connection>(new Connection(*this, endpoint));
  auto pair = _connections.emplace(make_pair(endpoint, move(c)));
  assert(pair.second);
  return pair.first;
}

void Node::connect(Endpoint remote_endpoint) {
  if (_connections.count(remote_endpoint)) return;
  create_connection(remote_endpoint);
}

void Node::connection_lost(Endpoint remote_endpoint) {
  _connections.erase(remote_endpoint);
  start_fast_mis();
}

bool Node::is_connected_to(Endpoint remote_endpoint) const {
  return _connections.count(ID(remote_endpoint)) != 0;
}

template<class Message, class... Args> void Node::broadcast_contenders(Args... args) {
  for (auto& pair : _connections) {
    auto& c = *pair.second;
    if (!c.is_contender) continue;
    c.schedule_send<Message>(args...);
  }
}

bool Node::has_number_from_all() const {
  bool retval = true;
  each_connection([&](const Connection& c) {
      if (!c.is_contender) return;
      if (!c.random_number) retval = false;
      });
  return retval;
}

bool Node::has_update1_from_all_contenders() const {
  bool retval = true;
  each_connection([&](const Connection& c) {
      if (!c.is_contender) return;
      if (!c.update1) retval = false;
      });
  return retval;
}

bool Node::has_update2_from_all_contenders() const {
  bool retval = true;
  each_connection([&](const Connection& c) {
      if (!c.is_contender) return;
      if (!c.update2) retval = false;
      });
  return retval;
}

bool Node::every_neighbor_decided() const {
  bool retval = true;
  each_connection([&](Connection& c) {
      if (!c.result || *c.result == LeaderStatus::undecided) {
        retval = false;
      }});
  return retval;
}

bool Node::smaller_than_others(float my_number) const {
  bool retval = true;
  each_connection([&](const Connection& c) {
      if (!c.is_contender) return;
      if (c.random_number && my_number >= c.random_number) {
        retval = false;
      }});
  return retval;
}

void Node::on_algorithm_completed() {
  assert(_fast_mis_started);
  _fast_mis_started = false;
  log(id(), " !!! ", _leader_status, " !!!");
  if (_on_algorithm_completed) {
    auto handler = _on_algorithm_completed;
    handler();
  }
}

bool Node::has_leader_neighbor() const {
  for (const auto& pair : _connections) {
    const auto& c = *pair.second;
    if (c.update1 && *c.update1 == LeaderStatus::leader) {
      return true;
    }
    if (c.update2 && *c.update2 == LeaderStatus::leader) {
      return true;
    }
  }
  return false;
}

void Node::start_fast_mis() {
  // This function get called either by the user or is triggered
  // when another node gets disconnected from the network. In the
  // second case it is possible that another node has detected
  // this disconnection sooner and already started the algorithm
  // by sending us the StartMsg message. In such case we've
  // already reset all these variables and they might have even
  // been set to new values by the algorithm, so it would
  // be a bug to reset them again.
  if (!_fast_mis_started) {
    for (const auto& pair : _connections) {
      auto& c = *pair.second;
      // TODO: This should be in Connection
      c.knows_my_result = false;
      c.random_number = false;
      c.update1.reset();
      c.update2.reset();
      c.result.reset();
      c.is_contender = true;
    }
    _state = numbers;
    reset_all_numbers();
    _leader_status = LeaderStatus::undecided;
    broadcast_contenders<StartMsg>();
  }

  _fast_mis_started = true;
  on_receive_number();
}

void Node::on_received_start() {
  if (_fast_mis_started) return;
  start_fast_mis();
}

void Node::on_receive_number() {
  if (_state != numbers) return;

  assert(_fast_mis_started);
  assert(_leader_status == LeaderStatus::undecided);

  if (!_my_random_number) {
    _my_random_number = Random::instance().generate_float();
    broadcast_contenders<NumberMsg>(*_my_random_number);
  }

  if (!has_number_from_all()) {
    return;
  }

  if (smaller_than_others(*_my_random_number)) {
    log(id(), " elected leader");
    _leader_status = LeaderStatus::leader;
  }

  // We don't need these anymore and they need
  // to be unsed for the next stage.
  reset_all_numbers();

  broadcast_contenders<Update1Msg>(_leader_status);

  _state = updates1;
  on_receive_update1();
}

void Node::on_receive_update1() {
  if (_state != updates1) return;

  if (!has_update1_from_all_contenders()) return;

  if (has_leader_neighbor()) {
    log(id(), " has leader neigbor => follower");
    _leader_status = LeaderStatus::follower;
  }

  each_connection([](Connection& c) { c.update1.reset(); });

  broadcast_contenders<Update2Msg>(_leader_status);

  _state = updates2;
  on_receive_update2();
}

void Node::on_receive_update2() {
  if (_state != updates2) return;

  if (!has_update2_from_all_contenders()) return;

  each_connection([&](Connection& c) {
      if (!c.is_contender) return;
      if (*c.update2 != LeaderStatus::undecided) {
        c.is_contender = false;
      }
      });

  each_connection([](Connection& c) { c.update2.reset(); });

  if (_leader_status != LeaderStatus::undecided) {
    _state = idle;
    each_connection([&](Connection& c) {
        if (c.knows_my_result) return;
        c.knows_my_result = true;
        c.schedule_send<ResultMsg>(_leader_status);
        });

    on_receive_result();
  }
  else {
    _state = numbers;
    on_receive_number();
  }
}

void Node::on_receive_result() {
  assert(_fast_mis_started);
  if (!every_neighbor_decided()) return;
  on_algorithm_completed();
}

void Node::reset_all_numbers() {
  _my_random_number.reset();
  each_connection([](Connection& c) { c.random_number.reset(); });
}

std::ostream& operator<<(std::ostream& os, const Node& node) {
  os << node.id() << "(" << node._leader_status << "): ";
  for (const auto& pair : node._connections) {
    os << pair.second->id() << " ";
  }
  return os;
}

Node::~Node() {}

