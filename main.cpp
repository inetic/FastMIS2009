#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include "Connection.h"

namespace po = boost::program_options;
namespace asio = boost::asio;
using namespace std;

asio::ip::udp::endpoint parse_endpoint(const string& ep_str) {
  auto begin     = ep_str.begin();
  auto end       = ep_str.end();
  auto delimiter = find(begin, end, ':');

  if (delimiter == end) {
    throw std::runtime_error("invalid endpoint format");
  }

  auto address = asio::ip::address::from_string(string(begin, delimiter)); 
  unsigned short port = stoi(string(++delimiter, end));

  return asio::ip::udp::endpoint(address, port);
}

int main(int ac, const char** av) {

  po::options_description desc("Allowed options");

  desc.add_options()
    ("help", "produce help message")
    ("peers", po::value<vector<string>>()->multitoken()
            , "peer endpoints in format IP:PORT");

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  vector<string> peers_str;
  
  if (vm.count("peers")) {
    peers_str = vm["peers"].as<vector<string>>();
  }

  asio::io_service ios;

  for (const auto& s : peers_str) {
    auto connection = make_shared<Connection>(ios);
    asio::ip::udp::endpoint ep = parse_endpoint(s);
    connection->start(ep);
  }

  ios.run();
}

