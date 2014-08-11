#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/optional.hpp>
#include "Endpoint.h"
#include "ID.h"
#include "PeriodicTimer.h"
#include "protocol.h"

class Node;

class Connection {
  using MessagePtr = std::unique_ptr<Message>;

public:
  Connection(Node&, Endpoint remote_endpoint);

  void receive_data(const std::string&);

  ID id() const { return ID(_remote_endpoint); }

  template<class Msg, class... Args> void schedule_send(Args... args) {
    Msg* msg = new Msg(++_tx_sequence_id, _rx_sequence_id, args...);
    _tx_messages.push_back(msg);
  }

private:
  void keep_alive();
  void on_tick();

  template<class Msg> void receive(const Msg&);

  void use_message(const PingMsg&);
  void use_message(const StartMsg&);
  void use_message(const NumberMsg&);
  void use_message(const Status1Msg&);
  void use_message(const Status2Msg&);

  void ack_message(uint32_t ack_sequence_number);

private:
  Node&           _node;
  const Endpoint  _remote_endpoint;
  PeriodicTimer   _periodic_timer;
  unsigned int    _missed_ping_count;

  boost::ptr_deque<Message> _tx_messages;
  uint32_t                  _rx_sequence_id;
  uint32_t                  _tx_sequence_id;

public:
  // FastMIS related data.
  boost::optional<float>        random_number;
  boost::optional<LeaderStatus> leader_status1;
  boost::optional<LeaderStatus> leader_status2;
};

#endif // ifndef __CONNECTION_H__
