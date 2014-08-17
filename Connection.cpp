#include <iostream>
#include <algorithm>
#include "Connection.h"
#include "Node.h"
#include "constants.h"
#include "log.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
namespace pstime = boost::posix_time;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
Connection::Connection(Node& node, Endpoint remote_endpoint)
  : _node(node)
  , _remote_endpoint(remote_endpoint)
  , _periodic_timer( node._ping_timeout
                   , node.get_io_service(), [=]() { on_tick(); })
  , _missed_ping_count(0)
  , _is_sending(false)
  , _rx_sequence_id(0)
  , _tx_sequence_id(0)
  , knows_my_result(false)
  , is_contender(false)
{
  // The first message to establish connection.
  schedule_send<PingMsg>();
}

//------------------------------------------------------------------------------
void Connection::send(const Message& msg) {
  if (_is_sending) return;
  _is_sending = true;

  stringstream ss;
  ss << msg.label() << " " << msg;

  auto destroyed = _destroy_guard.indicator();

  auto data = make_shared<string>(ss.str());
  _node.socket().async_send_to
    ( asio::buffer(*data)
    , _remote_endpoint
    , [this, data, destroyed](boost::system::error_code, size_t) {
      if (destroyed) return;
      _is_sending = false;
    });
}

void Connection::send_front_message() {
  _tx_messages.front().ack_sequence_number = _rx_sequence_id;
  send(_tx_messages.front());
}

//------------------------------------------------------------------------------
void Connection::on_tick() {
  ++_missed_ping_count;

  if (_missed_ping_count > _node._max_missed_ping_count) {
    // Disonnection will destroy this object, so make sure you
    // return immediately.
    _node.connection_lost(_remote_endpoint);
    return;
  }

  if (_missed_ping_count > 1) {
    increment_timer_duration();
  }

  if (!_tx_messages.empty()) {
    send_front_message();
  }
  else {
    send(PingMsg(_tx_sequence_id, _rx_sequence_id));
  }
}

//------------------------------------------------------------------------------
void Connection::use_message(const PingMsg&) {
}

//------------------------------------------------------------------------------
void Connection::use_message(const StartMsg&) {
  _node.on_received_start();
}

//------------------------------------------------------------------------------
void Connection::use_message(const NumberMsg& msg) {
  random_number.reset(msg.random_number);
  _node.on_receive_number();
}

//------------------------------------------------------------------------------
void Connection::use_message(const Update1Msg& msg) {
  update1 = msg.status;
  _node.on_receive_update1();
}

//------------------------------------------------------------------------------
void Connection::use_message(const Update2Msg& msg) {
  update2 = msg.status;
  _node.on_receive_update2();
}

//------------------------------------------------------------------------------
void Connection::use_message(const ResultMsg& msg) {
  result = msg.status;
  _node.on_receive_result();
}

//------------------------------------------------------------------------------
void Connection::keep_alive() {
  _missed_ping_count = 0;
  decrement_timer_duration();
}

//------------------------------------------------------------------------------
void Connection::ack_message(uint32_t ack_sequence_number) {
  if (_tx_messages.empty()) return;

  if (_tx_messages.front().sequence_number == ack_sequence_number) {
    _tx_messages.pop_front();
  }
}

//------------------------------------------------------------------------------
void Connection::increment_timer_duration() {
  _periodic_timer.set_duration(_periodic_timer.duration()*2);
}

//------------------------------------------------------------------------------
void Connection::decrement_timer_duration() {
  using namespace pstime;
  auto default_d = milliseconds(PING_TIMEOUT_MS);
  auto current_d = _periodic_timer.duration();

  if (current_d > default_d) {
    auto new_d = max<time_duration>(default_d, current_d - (default_d*0.5));
    _periodic_timer.set_duration(new_d);
  }
}

//------------------------------------------------------------------------------
ID Connection::node_id() const { return _node.id(); }

