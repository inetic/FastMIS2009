#include <iostream>
#include "Connection.h"
#include "Node.h"
#include "constants.h"

namespace asio = boost::asio;
using udp = asio::ip::udp;
using namespace std;
namespace pstime = boost::posix_time;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
Connection::Connection(Node& node, Endpoint remote_endpoint)
  : _node(node)
  , _remote_endpoint(remote_endpoint)
  , _periodic_timer(node.get_io_service(), [=]() { on_tick(); })
  , _missed_ping_count(0)
  , _rx_sequence_id(0)
  , _tx_sequence_id(0)
{
}

//------------------------------------------------------------------------------
void Connection::on_tick() {
  if (_missed_ping_count > MAX_MISSED_PING_COUNT) {
    // Disonnection will destroy this object, so make sure you
    // return immediately.
    _node.disconnect(_remote_endpoint);
    return;
  }
  ++_missed_ping_count;

  if (!_tx_messages.empty()) {
    _tx_messages.front().ack_sequence_number = _rx_sequence_id;
    _node.send_to(_tx_messages.front(), _remote_endpoint);
  }
  else {
    _node.send_to(PingMsg(_tx_sequence_id, _rx_sequence_id), _remote_endpoint);
  }
}

//------------------------------------------------------------------------------
void Connection::receive_data(const std::string& data) {
  std::stringstream ss(data);

  dispatch_message(ss
      , [&](const PingMsg& msg)    { receive(msg); }
      , [&](const StartMsg& msg)   { receive(msg); }
      , [&](const NumberMsg& msg)  { receive(msg); }
      , [&](const Status1Msg& msg) { receive(msg); }
      , [&](const Status2Msg& msg) { receive(msg); });
}

//------------------------------------------------------------------------------
template<class Msg> void Connection::receive(const Msg& msg) {
  if (msg.sequence_number == _rx_sequence_id + 1) {
    keep_alive();
    _rx_sequence_id = msg.sequence_number;
    ack_message(msg.ack_sequence_number);
    use_message(msg);
  }
  else if (msg.sequence_number == _rx_sequence_id) {
    keep_alive();
  }
}

//------------------------------------------------------------------------------
void Connection::use_message(const PingMsg&) {
}

//------------------------------------------------------------------------------
void Connection::use_message(const StartMsg& msg) {
  //cout << _node.id() << " <- " << id() << " " << msg.label() << " " << msg << endl;
  _node.on_received_start();
}

//------------------------------------------------------------------------------
void Connection::use_message(const NumberMsg& msg) {
  //cout << _node.id() << " <- " << id() << " " << msg.label() << " " << msg << endl;
  random_number.reset(msg.random_number);
  _node.on_receive_number();
}

//------------------------------------------------------------------------------
void Connection::use_message(const Status1Msg& msg) {
  //cout << _node.id() << " <- " << id() << " " << msg.label() << " " << msg << endl;
  leader_status1 = msg.leader_status;
  _node.on_status1_changed();
}

//------------------------------------------------------------------------------
void Connection::use_message(const Status2Msg& msg) {
  //cout << _node.id() << " <- " << id() << " " << msg.label() << " " << msg << endl;
  leader_status2 = msg.leader_status;
  _node.on_status2_changed();
}

//------------------------------------------------------------------------------
void Connection::keep_alive() {
  _missed_ping_count = 0;
}

//------------------------------------------------------------------------------
void Connection::ack_message(uint32_t ack_sequence_number) {
  if (_tx_messages.empty()) return;

  if (_tx_messages.front().sequence_number == ack_sequence_number) {
    cout << _node.id() << " acked " << _tx_messages.front().label() << " ";
    _tx_messages.front().to_stream(cout);
    cout << "\n";
    _tx_messages.pop_front();
  }
}

