#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/asio.hpp>
#include "Graph.h"
#include "../Node.h"

class Network {
  using Nodes = boost::ptr_vector<Node>;

public:
  Network(boost::asio::io_service&);

  Network(Network&&) = default;
  Network(const Network&) = delete;
  Network& operator=(const Network&) = delete;

  void generate_connected(size_t node_count);
  void add_nodes(size_t node_count);
  void shutdown();
  bool is_MIS() const;

  bool every_node_stopped() const;
  bool every_node_decided() const;
  bool every_neighbor_decided() const;

  Node& operator[](size_t i) { return _nodes[i]; }

  size_t size() const { return _nodes.size(); }

  void start_fast_mis(const std::function<void()>&);
  void start_fast_mis();

  void extract_connected(Network&);

  bool empty() const { return _nodes.empty(); }

  Graph build_graph() const;

private:
  void extract_connected(Network&, Nodes::iterator);

private:
  friend std::ostream& operator<<(std::ostream&, const Network&);

  boost::asio::io_service& _io_service;
  Nodes                    _nodes;
};

std::ostream& operator<<(std::ostream& os, const Network&);

#endif // ifndef __NETWORK_H__
