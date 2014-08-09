#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include "Node.h"
#include "constants.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using namespace std;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(one_node) {
  asio::io_service ios;

  Node node(ios);
  node.start();

  asio::deadline_timer timer(ios, pstime::milliseconds(2*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      node.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);

  node0.start();
  node1.start();

  asio::deadline_timer timer(ios, pstime::milliseconds(2*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      node0.shutdown();
      node1.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_connected_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);

  node0.start();
  node1.start();

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, pstime::milliseconds(3*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1.local_endpoint()));
      BOOST_REQUIRE(node1.is_connected_to(node0.local_endpoint()));

      node0.shutdown();
      node1.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(disconnect_two_connected_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);

  node0.start();
  node1.start();

  auto node0_ep = node0.local_endpoint();
  auto node1_ep = node1.local_endpoint();

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, pstime::milliseconds(5*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));

      node0.shutdown();

      timer.expires_from_now(
        pstime::milliseconds(2*PING_TIMEOUT_MS*MAX_MISSED_PING_COUNT));

      timer.async_wait([&](Error) {
        BOOST_REQUIRE(!node1.is_connected_to(node0_ep));
        node1.shutdown();
        });
      });

  ios.run();
}

//------------------------------------------------------------------------------

