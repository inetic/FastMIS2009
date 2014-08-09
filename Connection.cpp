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
    _node.send_to(_tx_messages.front(), _remote_endpoint);
  }
  else {
    _node.send_to(Ping(_tx_sequence_id, _rx_sequence_id), _remote_endpoint);
  }
}

//------------------------------------------------------------------------------
template<class Msg, class... Args>
void Connection::schedule_send(Args... args) {
  _tx_messages.push_back(new Msg( ++_tx_sequence_id
                                , _rx_sequence_id
                                , args...));
}

void Connection::receive_data(const std::string& data) {
  std::stringstream ss(data);

  dispatch_message(ss
      , [&](const Ping& ping) { receive(ping); });
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
void Connection::use_message(const Ping& ping) {
  cout << "Received ping " << ping << endl;
}

//------------------------------------------------------------------------------
void Connection::keep_alive() {
  _missed_ping_count = 0;
}

//------------------------------------------------------------------------------
void Connection::ack_message(uint32_t ack_sequence_number) {
  if (_tx_messages.empty()) return;

  if (_tx_messages.front().sequence_number == ack_sequence_number) {
    _tx_messages.pop_front();
  }
}

