#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(boost::asio::io_service&);

  void start(const boost::asio::ip::udp::endpoint& remote_endpoint);

private:
  void shutdown();

private:
  boost::asio::io_service&     _io_service;
  boost::asio::ip::udp::socket _socket;
};

#endif // ifndef __CONNECTION_H__
