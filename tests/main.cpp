#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include "Graph.h"
#include "constants.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using milliseconds = pstime::milliseconds;
using namespace std;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(one_node) {
  asio::io_service ios;

  Node node(ios);

  asio::deadline_timer timer(ios, milliseconds(2*PING_TIMEOUT_MS));

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

  asio::deadline_timer timer(ios, milliseconds(2*PING_TIMEOUT_MS));

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

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, milliseconds(3*PING_TIMEOUT_MS));

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

  auto node0_ep = node0.local_endpoint();
  auto node1_ep = node1.local_endpoint();

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, milliseconds(5*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));

      node0.shutdown();

      timer.expires_from_now(
        milliseconds(2*PING_TIMEOUT_MS*MAX_MISSED_PING_COUNT));

      timer.async_wait([&](Error) {
        BOOST_REQUIRE(!node1.is_connected_to(node0_ep));
        node1.shutdown();
        });
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(three_connected_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);
  Node node2(ios);

  node0.connect(node1.local_endpoint());
  node1.connect(node2.local_endpoint());

  asio::deadline_timer timer(ios, milliseconds(3*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1.local_endpoint()));
      BOOST_REQUIRE(node1.is_connected_to(node0.local_endpoint()));
      BOOST_REQUIRE(node1.is_connected_to(node2.local_endpoint()));
      BOOST_REQUIRE(node2.is_connected_to(node1.local_endpoint()));

      node0.shutdown();
      node1.shutdown();
      node2.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(disconnect_three_connected_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);
  Node node2(ios);

  auto node0_ep = node0.local_endpoint();
  auto node1_ep = node1.local_endpoint();
  auto node2_ep = node2.local_endpoint();

  node0.connect(node1.local_endpoint());
  node1.connect(node2.local_endpoint());

  asio::deadline_timer timer(ios, milliseconds(5*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));
      BOOST_REQUIRE(node1.is_connected_to(node2_ep));
      BOOST_REQUIRE(node2.is_connected_to(node1_ep));

      node0.shutdown();

      timer.expires_from_now(
        milliseconds(2*PING_TIMEOUT_MS*MAX_MISSED_PING_COUNT));

      timer.async_wait([&](Error) {
        BOOST_REQUIRE(!node0.is_connected_to(node1_ep));
        BOOST_REQUIRE(node1.is_connected_to(node2_ep));

        node1.shutdown();
        node2.shutdown();
        });
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_nodes_fast_mis) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);

  auto node0_ep = node0.local_endpoint();
  auto node1_ep = node1.local_endpoint();

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios);

  node0.start_fast_mis([&]() {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));

      // Give time to other nodes to decide.
      timer.expires_from_now(
        milliseconds(PING_TIMEOUT_MS*MAX_MISSED_PING_COUNT));

      timer.async_wait([&](Error) {
        node0.shutdown();
        node1.shutdown();
        });
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(graph) {
  while (true) {
    asio::io_service ios;

    Graph graph(ios);

    graph.generate_connected(4);

    cout << "----------------------------------" << endl;
    cout << graph << endl;
    cout << "----------------------------------" << endl;

    asio::deadline_timer timer(ios);

    graph[0].start_fast_mis([&]() {
        // Give time to other nodes to decide.
        timer.expires_from_now(
          milliseconds(PING_TIMEOUT_MS*MAX_MISSED_PING_COUNT));

        cout << "=============== done\n";
        timer.async_wait([&](Error) {
          cout << "Shutting down\n";
          graph.shutdown();
          });
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------

