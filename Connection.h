#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include "Endpoint.h"
#include "PeriodicTimer.h"

class Node;

class Connection {
public:
  Connection(Node&, Endpoint remote_endpoint);

private:
  void on_tick();

private:
  Node&               _node;
  const Endpoint      _remote_endpoint;
  PeriodicTimer       _periodic_timer;
};

#endif // ifndef __CONNECTION_H__
