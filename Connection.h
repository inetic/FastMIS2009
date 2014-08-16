#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/optional.hpp>
#include "Endpoint.h"
#include "DestroyGuard.h"
#include "ID.h"
#include "PeriodicTimer.h"
#include "protocol.h"

class Node;

class Connection {
  using MessagePtr = std::unique_ptr<Message>;

public:
  Connection(Node&, Endpoint remote_endpoint);

  Connection(const Connection&)                  = delete;
  const Connection& operator=(const Connection&) = delete;

  ID id() const { return ID(_remote_endpoint); }
  ID node_id() const;

  template<class Msg, class... Args>
  void schedule_send(Args... args) {
    bool was_empty = _tx_messages.empty();
    Msg* msg = new Msg(++_tx_sequence_id, _rx_sequence_id, args...);
    log(node_id(), " -> ", id(), " ", msg->label(), " ", *msg);
    _tx_messages.push_back(msg);
    if (was_empty) send_front_message();
  }

  template<class Msg> void receive(const Msg& msg) {
    assert(msg.sequence_number <= _rx_sequence_id + 1);

    ack_message(msg.ack_sequence_number);

    if (msg.sequence_number == _rx_sequence_id + 1) {
      // DEBUG
      if (msg.label() != "ping") {
        log(node_id(), " <- ", id(), " ", msg.label(), " ", msg);
      }

      keep_alive();
      _rx_sequence_id = msg.sequence_number;
      use_message(msg);
    }
    else if (msg.sequence_number == _rx_sequence_id) {
      keep_alive();
    }
  }

private:
  void keep_alive();
  void on_tick();

  void use_message(const PingMsg&);
  void use_message(const StartMsg&);
  void use_message(const NumberMsg&);
  void use_message(const Update1Msg&);
  void use_message(const Update2Msg&);
  void use_message(const ResultMsg&);

  void ack_message(uint32_t ack_sequence_number);

  void send(const Message& msg);
  void send_front_message();

private:
  Node&           _node;
  const Endpoint  _remote_endpoint;
  PeriodicTimer   _periodic_timer;
  unsigned int    _missed_ping_count;
  bool            _is_sending;

  boost::ptr_deque<Message> _tx_messages;
  uint32_t                  _rx_sequence_id;
  uint32_t                  _tx_sequence_id;

  DestroyGuard              _destroy_guard;

public:
  // FastMIS related data.
  bool                          knows_my_result;
  boost::optional<float>        random_number;
  boost::optional<LeaderStatus> update1;
  boost::optional<LeaderStatus> update2;
  boost::optional<LeaderStatus> result;
  bool                          is_contender;
};

#endif // ifndef __CONNECTION_H__
