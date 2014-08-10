#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/optional.hpp>
#include "Endpoint.h"
#include "PeriodicTimer.h"
#include "protocol.h"

class Node;

class Connection {
  using MessagePtr = std::unique_ptr<Message>;

public:
  Connection(Node&, Endpoint remote_endpoint);

  void receive_data(const std::string&);

  void start_fast_mis(float r);

private:
  void keep_alive();
  void on_tick();

  template<class Msg> void receive(const Msg&);

  void use_message(const Ping&);
  void use_message(const Start&);

  void ack_message(uint32_t ack_sequence_number);

  template<class Msg, class... Args> void schedule_send(Args...);

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
  boost::optional<float> random_number;
  bool                   knows_my_number = false;
};

#endif // ifndef __CONNECTION_H__
