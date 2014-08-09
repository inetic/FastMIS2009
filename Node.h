#ifndef __NODE_H__
#define __NODE_H__

#include <map>
#include <boost/uuid/uuid.hpp>
#include "Endpoint.h"
#include "ID.h"

class Connection;
class Message;

class Node {
private:
  using ConnectionPtr = std::shared_ptr<Connection>;

public:
  Node(boost::asio::io_service& io_service);

  void start();
  void shutdown();
  void connect(Endpoint);
  void disconnect(Endpoint);

  Endpoint local_endpoint() const { return _socket.local_endpoint(); }
  boost::asio::io_service& get_io_service() { return _io_service; }

  bool is_connected_to(Endpoint) const;

  void send_to(const Message& msg, Endpoint destination);

private:
  void receive_data();
  void use_data(Endpoint sender, std::string&&);
  Connection& create_connection(Endpoint);

private:
  boost::uuids::uuid            _uuid;
  boost::asio::io_service&      _io_service;
  boost::asio::ip::udp::socket  _socket;
  std::map<ID, ConnectionPtr>   _connections;
};

#endif // ifndef __NODE_H__
