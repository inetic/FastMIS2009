#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include "Random.h"
#include "Graph.h"
#include "constants.h"
#include "log.h"
#include "WhenAll.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using milliseconds = pstime::milliseconds;
using namespace std;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
// This tests whether shutting down one node terminates it, thus no asserts.
BOOST_AUTO_TEST_CASE(one_node_shutdown) {
  asio::io_service ios;

  Node node(ios);

  asio::deadline_timer timer(ios, milliseconds(2*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      node.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
// This tests whether shutting down nodes terminates them, thus no asserts.
BOOST_AUTO_TEST_CASE(two_nodes_shutdown) {
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

  // Makes the test finish quicker
  unsigned int max_missed_ping_count = 5;
  milliseconds ping_timeout(30);

  node1.set_ping_timeout(ping_timeout);
  node1.set_max_missed_ping_count(max_missed_ping_count);

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, ping_timeout*5);

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));

      node0.shutdown();

      timer.expires_from_now(ping_timeout * max_missed_ping_count * 2);

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

  // Makes the test finish quicker
  unsigned int max_missed_ping_count = 5;
  milliseconds ping_timeout(30);

  node1.set_ping_timeout(ping_timeout);
  node1.set_max_missed_ping_count(max_missed_ping_count);
  node2.set_ping_timeout(ping_timeout);
  node2.set_max_missed_ping_count(max_missed_ping_count);

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

      timer.expires_from_now(ping_timeout * max_missed_ping_count * 2);

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

  WhenAll when_all([&]() {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));
      node0.shutdown();
      node1.shutdown();
      });

  node0.on_fast_mis_ended(when_all.make_continuation());
  node1.on_fast_mis_ended(when_all.make_continuation());

  node0.start_fast_mis();

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(many_3_node_graphs) {
  for (unsigned int i = 0; i < 20; i++) {
    Random::instance().initialize_with_random_seed();

    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Graph graph(ios);

    graph.generate_connected(5);

    log("----------------------------------");
    log(graph);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    graph.start_fast_mis([&]() {
        BOOST_REQUIRE(graph.every_node_stopped());
        BOOST_REQUIRE(graph.every_node_decided());
        BOOST_REQUIRE(graph.every_neighbor_decided());
        graph.shutdown();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(many_5_node_graphs) {
  for (unsigned int i = 0; i < 20; i++) {
    Random::instance().initialize_with_random_seed();

    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Graph graph(ios);

    graph.generate_connected(5);

    log("----------------------------------");
    log(graph);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    graph.start_fast_mis([&]() {
        BOOST_REQUIRE(graph.every_node_stopped());
        BOOST_REQUIRE(graph.every_node_decided());
        BOOST_REQUIRE(graph.every_neighbor_decided());
        graph.shutdown();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------

