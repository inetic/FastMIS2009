#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <cmath>
#include "Random.h"
#include "Network.h"
#include "constants.h"
#include "log.h"
#include "WhenAll.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using milliseconds = pstime::milliseconds;
using namespace std;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
// Test if graph tests are correct.
BOOST_AUTO_TEST_CASE(graph_tests) {
  { // 0 nodes; 0 edges
    Graph g;
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 1 nodes; 0 edges; 0
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 1 nodes; 0 edges; 1
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 2 nodes; 0 edges; 0
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::follower);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 2 nodes; 1 edges
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.connect(0,1);
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 2 nodes; 1 edges
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.connect(0,1);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 0
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 1
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 2
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 3 nodes; 2 edges; 3
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 4
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::leader);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 5
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::leader);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 3 nodes; 2 edges; 6
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::leader);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 2 edges; 7
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::leader);
    g.connect(0,1);
    g.connect(1,2);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 3 edges; 0 leaders
    Graph g;
    g.nodes.emplace(0, LeaderStatus::follower);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    g.connect(2,0);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 3 edges; 1 leader
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::follower);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    g.connect(2,0);
    BOOST_REQUIRE(g.is_MIS());
  }

  { // 3 nodes; 3 edges; 2 leaders
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::follower);
    g.connect(0,1);
    g.connect(1,2);
    g.connect(2,0);
    BOOST_REQUIRE(!g.is_MIS());
  }

  { // 3 nodes; 3 edges; 3 leaders
    Graph g;
    g.nodes.emplace(0, LeaderStatus::leader);
    g.nodes.emplace(1, LeaderStatus::leader);
    g.nodes.emplace(2, LeaderStatus::leader);
    g.connect(0,1);
    g.connect(1,2);
    g.connect(2,0);
    BOOST_REQUIRE(!g.is_MIS());
  }
}

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
  unsigned int max_missed_ping_count = 3;
  milliseconds ping_timeout(20);

  node1.set_ping_timeout(ping_timeout);
  node1.set_max_missed_ping_count(max_missed_ping_count);

  node0.connect(node1.local_endpoint());

  asio::deadline_timer timer(ios, ping_timeout*max_missed_ping_count);

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));

      node0.shutdown();

      timer.expires_from_now
        (ping_timeout * pow(2, max_missed_ping_count+2));

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
  unsigned int max_missed_ping_count = 3;
  milliseconds ping_timeout(20);

  node1.set_ping_timeout(ping_timeout);
  node1.set_max_missed_ping_count(max_missed_ping_count);
  node2.set_ping_timeout(ping_timeout);
  node2.set_max_missed_ping_count(max_missed_ping_count);

  auto node0_ep = node0.local_endpoint();
  auto node1_ep = node1.local_endpoint();
  auto node2_ep = node2.local_endpoint();

  node0.connect(node1.local_endpoint());
  node1.connect(node2.local_endpoint());

  asio::deadline_timer timer(ios, milliseconds(3*PING_TIMEOUT_MS));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1_ep));
      BOOST_REQUIRE(node1.is_connected_to(node0_ep));
      BOOST_REQUIRE(node1.is_connected_to(node2_ep));
      BOOST_REQUIRE(node2.is_connected_to(node1_ep));

      node0.shutdown();

      timer.expires_from_now
        (ping_timeout * pow(2, max_missed_ping_count + 1));

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
BOOST_AUTO_TEST_CASE(one_node_networks) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(1, 0);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    network.start_fast_mis([&]() {
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());
        network.shutdown();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(many_3_node_networks) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(3, 1.5);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    network.start_fast_mis([&]() {
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());
        network.shutdown();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(many_5_node_networks) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(5, 2.5);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    network.start_fast_mis([&]() {
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());
        network.shutdown();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(big_network) {
  Random::instance().initialize_with_random_seed();
  log("New seed: ", Random::instance().get_seed());

  asio::io_service ios;

  Network network(ios);

  auto& random = Random::instance();
  network.generate_connected(random.generate_int(200, 300), 5);

  log("----------------------------------");
  log(network);
  log("----------------------------------");

  asio::deadline_timer timer(ios);

  network.start_fast_mis([&]() {
      BOOST_REQUIRE(network.every_node_stopped());
      BOOST_REQUIRE(network.every_node_decided());
      BOOST_REQUIRE(network.every_neighbor_decided());
      BOOST_REQUIRE(network.is_MIS());
      network.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multirun) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(5, 2.5);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    int iterations_left = 5;

    network.start_fast_mis([&]() {
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());

        if (--iterations_left <= 0) {
          network.shutdown();
          return;
        }

        network.start_fast_mis();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(concurrent_starts) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(7, 3.5);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    WhenAll when_all([&]() {
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());

        network.shutdown();
        });

    for (auto& node : network) {
      node.on_fast_mis_ended(when_all.make_continuation());
    }

    network[0].start_fast_mis();
    network[1].start_fast_mis();
    network[2].start_fast_mis();

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(add_nodes) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    const int node_start_count = 5;
    network.generate_connected(node_start_count, (float) node_start_count / 2);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    const int iteration_total = 5;
    int iteration = 0;

    network.start_fast_mis([&]() {
        BOOST_REQUIRE_EQUAL(network.size(), node_start_count + iteration);
        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());

        if (++iteration == iteration_total) {
          network.shutdown();
          return;
        }

        network.add_random_node();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(remove_nodes) {
  for (unsigned int i = 0; i < 10; i++) {
    Random::instance().initialize_with_random_seed();
    log("New seed: ", Random::instance().get_seed());

    asio::io_service ios;

    Network network(ios);

    network.generate_connected(5, 2.5);

    network.set_ping_timeout(milliseconds(10));
    network.set_max_missed_ping_count(2);

    log("----------------------------------");
    log(network);
    log("----------------------------------");

    asio::deadline_timer timer(ios);

    network.start_fast_mis([&]() {
        network.remove_dead_nodes();

        BOOST_REQUIRE(network.every_node_stopped());
        BOOST_REQUIRE(network.every_node_decided());
        BOOST_REQUIRE(network.every_neighbor_decided());
        BOOST_REQUIRE(network.is_MIS());

        if (network.empty()) {
          network.shutdown();
          return;
        }

        network.remove_singletons(); // They would not trigger re-election.

        network.shutdown_random_node();
        });

    ios.run();
  }
}

//------------------------------------------------------------------------------

