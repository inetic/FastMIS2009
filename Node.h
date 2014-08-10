#ifndef __NODE_H__
#define __NODE_H__

#include <map>
#include <boost/uuid/uuid.hpp>
#include <boost/optional.hpp>
#include "Endpoint.h"
#include "ID.h"

class Connection;
class Message;

class Node {
private:
  using ConnectionPtr = std::shared_ptr<Connection>;
  // TODO: Do I need to store connections as pointers?
  using Connections   = std::map<ID, ConnectionPtr>;

public:
  Node(boost::asio::io_service& io_service);

  ID   id() const { return ID(_socket.local_endpoint()); }

  void start();
  void shutdown();
  void connect(Endpoint);
  void disconnect(Endpoint);

  Endpoint local_endpoint() const { return _socket.local_endpoint(); }
  boost::asio::io_service& get_io_service() { return _io_service; }

  bool is_connected_to(Endpoint) const;

  void send_to(const Message& msg, Endpoint destination);

  ~Node();

  void start_fast_mis();

private:
  void receive_data();
  void use_data(Endpoint sender, std::string&&);
  Connection& create_connection(Endpoint);

  friend class Connection;

  bool smaller_than_others(float) const;
  bool received_random_number_from_all_peers() const;

private:
  boost::uuids::uuid            _uuid;
  boost::asio::io_service&      _io_service;
  boost::asio::ip::udp::socket  _socket;
  Connections                   _connections;

  // FastMIS related data.
  boost::optional<float> _my_random_number;
};

#endif // ifndef __NODE_H__
