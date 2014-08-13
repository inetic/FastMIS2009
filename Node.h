#ifndef __NODE_H__
#define __NODE_H__

#include <map>
#include <set>
#include <boost/uuid/uuid.hpp>
#include <boost/optional.hpp>
#include "Endpoint.h"
#include "ID.h"
#include "LeaderStatus.h"

class Connection;
class Message;

class Node {
private:
  using ConnectionPtr = std::shared_ptr<Connection>;
  // TODO: Do I need to store connections as pointers?
  using Connections   = std::map<ID, ConnectionPtr>;

public:
  Node(boost::asio::io_service& io_service);

  Node(Node&&) = default;
  Node(const Node&) = delete;
  const Node& operator=(const Node&) = delete;

  ID id() const { return _id; }

  void shutdown();
  void connect(Endpoint);
  void disconnect(Endpoint);

  Endpoint local_endpoint() const { return _socket.local_endpoint(); }
  boost::asio::io_service& get_io_service() { return _io_service; }

  bool is_connected_to(Endpoint) const;

  ~Node();

  template<class Handler> void start_fast_mis(const Handler& handler) {
    on_fast_mis_ended(handler);
    start_fast_mis();
  }

  void start_fast_mis();

  template<class Handler> void on_fast_mis_ended(const Handler& handler) {
    _on_algorithm_completed = handler;
  }

  bool is_running_mis() const { return _fast_mis_started; }

  LeaderStatus leader_status() const { return _leader_status; }

  bool every_neighbor_decided() const;

private:
  void receive_data();
  void use_data(Endpoint sender, std::string&&);
  Connection& create_connection(Endpoint);

  void on_receive_number();
  void on_received_start();
  void on_receive_update1();
  void on_receive_update2();
  void on_receive_result();

  friend class Connection;

  bool smaller_than_others(float) const;
  bool has_number_from_all() const;
  bool has_update1_from_all_contenders() const;
  bool has_update2_from_all_contenders() const;
  bool has_result_from_all_connections() const;
  bool has_leader_neighbor() const;


  void on_algorithm_completed();

  template<class Message, class... Args> void broadcast(Args...);
  template<class Message, class... Args> void broadcast_contenders(Args...);

  template<class F> void each_connection(const F&);
  template<class F> void each_connection(const F&) const;

  boost::asio::ip::udp::socket& socket() { return _socket; }

  void reset_all_numbers();

private:
  friend std::ostream& operator<<(std::ostream&, const Node&);

  boost::asio::io_service&      _io_service;
  boost::asio::ip::udp::socket  _socket;
  ID                            _id;
  Connections                   _connections;
  bool                          _was_shut_down;

  // FastMIS related data.
  LeaderStatus           _leader_status = LeaderStatus::undecided;
  bool                   _fast_mis_started = false;
  //std::set<ID>           _contenders;
  boost::optional<float> _my_random_number;
  std::function<void()>  _on_algorithm_completed;
};

std::ostream& operator<<(std::ostream& os, const Node&);

#endif // ifndef __NODE_H__
