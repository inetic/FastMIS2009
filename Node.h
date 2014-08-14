#ifndef __NODE_H__
#define __NODE_H__

#include <map>
#include <set>
#include <boost/uuid/uuid.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Endpoint.h"
#include "ID.h"
#include "LeaderStatus.h"

class Connection;
class Message;

class Node {
private:
  enum State { idle, numbers, updates1, updates2 };

  using ConnectionPtr = std::shared_ptr<Connection>;
  // TODO: Do I need to store connections as pointers?
  using Connections = std::map<ID, ConnectionPtr>;
  using Duration    = boost::posix_time::time_duration;

public:
  Node(boost::asio::io_service& io_service);

  // Node is not movable because elements of _connections
  // hold a reference to this node.
  Node(Node&&)                       = delete;
  Node(const Node&)                  = delete;
  const Node& operator=(const Node&) = delete;

  ID id() const { return _id; }

  void shutdown();
  void connect(Endpoint);
  void disconnect(Endpoint);

  Endpoint local_endpoint() const { return _socket.local_endpoint(); }
  boost::asio::io_service& get_io_service() { return _io_service; }

  bool is_connected_to(Endpoint) const;

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

  void set_ping_timeout(Duration duration) { _ping_timeout = duration; }
  void set_max_missed_ping_count(size_t n) { _max_missed_ping_count = n; }

  template<class F> void each_connection(const F&f)       { for (auto p : _connections) { f(*p.second); } }
  template<class F> void each_connection(const F&f) const { for (auto p : _connections) { f(*p.second); } }

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

  template<class Message, class... Args> void broadcast_contenders(Args...);

  boost::asio::ip::udp::socket& socket() { return _socket; }

  void reset_all_numbers();

private:
  friend std::ostream& operator<<(std::ostream&, const Node&);

  boost::asio::io_service&      _io_service;
  boost::asio::ip::udp::socket  _socket;
  ID                            _id;
  Connections                   _connections;
  bool                          _was_shut_down;

  Duration      _ping_timeout;
  unsigned int  _max_missed_ping_count;

  // FastMIS related data.
  State                  _state;
  LeaderStatus           _leader_status = LeaderStatus::undecided;
  bool                   _fast_mis_started = false;
  boost::optional<float> _my_random_number;
  std::function<void()>  _on_algorithm_completed;
};

std::ostream& operator<<(std::ostream& os, const Node&);

#endif // ifndef __NODE_H__
