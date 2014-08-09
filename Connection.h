#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include "Endpoint.h"

class Connection {
public:
  Connection(Endpoint remote_endpoint);

private:

private:
  const Endpoint _remote_endpoint;
};

#endif // ifndef __CONNECTION_H__
