#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

#include <boost/asio/ip/udp.hpp>

using Endpoint = boost::asio::ip::udp::endpoint;

inline Endpoint parse_endpoint(const std::string& ep_str) {

  namespace asio = boost::asio;
  using namespace std;

  auto begin     = ep_str.begin();
  auto end       = ep_str.end();
  auto delimiter = find(begin, end, ':');

  if (delimiter == end) {
    throw std::runtime_error("invalid endpoint format");
  }

  auto address = asio::ip::address::from_string(string(begin, delimiter)); 
  unsigned short port = stoi(string(++delimiter, end));

  return Endpoint(address, port);
}


#endif // ifndef __ENDPOINT_H__
