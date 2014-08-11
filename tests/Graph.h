#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/asio.hpp>
#include "../Node.h"

class Graph {
public:
  Graph(boost::asio::io_service&);

  void generate_connected(size_t node_count);
  void add_nodes(size_t node_count);
  void shutdown();
  bool is_MIS() const;

  Node& operator[](size_t i) { return _nodes[i]; }

private:
  friend std::ostream& operator<<(std::ostream&, const Graph&);

  boost::asio::io_service& _io_service;
  boost::ptr_vector<Node>  _nodes;
};

std::ostream& operator<<(std::ostream& os, const Graph&);

#endif // ifndef __GRAPH_H__
