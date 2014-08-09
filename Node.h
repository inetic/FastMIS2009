#ifndef __NODE_H__
#define __NODE_H__

#include <list>
#include "Endpoint.h"
#include "Connection.h"

class Node {
  using ConnectionPtr = std::shared_ptr<Connection>;

public:
  Node(boost::asio::io_service& io_service);

  void start();
  void shutdown();
  void connect_to(Endpoint);
  boost::asio::io_service& get_io_service() { return _io_service; }

private:
  void receive_data();
  void use_data(Endpoint sender, std::string&&);
  Connection& create_connection(Endpoint);

private:
  boost::asio::io_service&          _io_service;
  boost::asio::ip::udp::socket      _socket;
  std::map<Endpoint, ConnectionPtr> _connections;
};

#endif // ifndef __NODE_H__
