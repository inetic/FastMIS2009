#ifndef __ID_H__
#define __ID_H__

#include <boost/asio/ip/udp.hpp>

// TODO: I'm not sure how safe it is to use endpoint as an ID yet.
//       Will have to think about it, but right now it's the easiest
//       option. If its only to be run on a local network, then it
//       should be safe (?).
class ID {
public:
  ID() {}
  ID(boost::asio::ip::udp::endpoint ep)
  {
    if (ep.address().is_unspecified()) {
      ep.address(boost::asio::ip::address_v4::loopback());
    }
    endpoint = ep;
  }

  bool operator<(const ID& id) const {
    return endpoint < id.endpoint;
  }

private:
  boost::asio::ip::udp::endpoint endpoint;
};

#endif // ifndef __ID_H__
