#ifndef __ID_H__
#define __ID_H__

#include <boost/asio/ip/udp.hpp>

// TODO: I'm not sure how safe it is to use endpoint as an ID yet.
//       Will have to think about it, but right now it's the easiest
//       option. If its only to be run on a local network, then it
//       should be safe (?).
class ID {
  using UDP = boost::asio::ip::udp;

public:
  ID() {}

  // For testing only
  ID(unsigned short i) : ID(UDP::endpoint(UDP::v4(), i)) { }

  ID(UDP::endpoint ep)
  {
    if (ep.address().is_unspecified()) {
      ep.address(boost::asio::ip::address_v4::loopback());
    }
    endpoint = ep;
  }

  bool operator<(const ID& id) const {
    return endpoint < id.endpoint;
  }

  bool operator==(const ID& id) const {
    return endpoint == id.endpoint;
  }

private:
  friend std::ostream& operator<<(std::ostream&, const ID&);

  boost::asio::ip::udp::endpoint endpoint;
};

#ifndef NDEBUG
inline std::ostream& operator<<(std::ostream& os, const ID& id) {
  return os << id.endpoint.port();
}

#endif

#endif // ifndef __ID_H__
