#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include "Node.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using namespace std;
using Error = boost::system::error_code;

//------------------------------------------------------------------------------
void kill_node_after_timeout(Node& node) {
  auto timer = make_shared<asio::deadline_timer>( node.get_io_service()
                                                , pstime::seconds(1));

  timer->async_wait([timer, &node](Error) {
      node.shutdown();
      });
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(one_node) {
  asio::io_service ios;

  Node node(ios);
  node.start();

  kill_node_after_timeout(node);

  ios.run();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_nodes) {
  asio::io_service ios;

  Node node0(ios);
  Node node1(ios);

  node0.start();
  node1.start();

  kill_node_after_timeout(node0);
  kill_node_after_timeout(node1);

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

  asio::deadline_timer timer(ios, pstime::milliseconds(500));

  timer.async_wait([&](Error) {
      BOOST_REQUIRE(node0.is_connected_to(node1.local_endpoint()));
      BOOST_REQUIRE(node1.is_connected_to(node0.local_endpoint()));

      node0.shutdown();
      node1.shutdown();
      });

  ios.run();
}

//------------------------------------------------------------------------------

