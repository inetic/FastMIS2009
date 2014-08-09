#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastMIS2009

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Node.h"

namespace asio = boost::asio;
namespace pstime = boost::posix_time;
using namespace std;

//------------------------------------------------------------------------------
void kill_node_after_timeout(Node& node) {
  auto timer = make_shared<asio::deadline_timer>( node.get_io_service()
                                                , pstime::seconds(1));

  timer->async_wait([timer, &node](boost::system::error_code ec) {
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

