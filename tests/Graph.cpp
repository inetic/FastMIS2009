#include <set>
#include <iostream>
#include <boost/random/random_device.hpp>
#include "Random.h"
#include "Graph.h"
#include "../Node.h"

using namespace boost;
using namespace std;

static unsigned int get_random_number(int max) {
  if (max == 0) { return 0; }
  return Random::instance().generate_int(0, max - 1);
  //typedef boost::random_device Dev;
  //Dev generate;
  //Dev::result_type random_number = generate();
  //return random_number - Dev::min();
}

Graph::Graph(asio::io_service& ios)
  : _io_service(ios)
{}

void Graph::add_nodes(size_t node_count) {
  for (size_t i = 0; i < node_count; ++i) {
    _nodes.push_back(new Node(_io_service));
  }
}

void Graph::generate_connected(size_t node_count) {
  add_nodes(node_count);

  if (node_count <= 1) return;

  set<pair<size_t, size_t>> connections;

  // Generate minimal connected graph
  for (size_t i = 0; i < _nodes.size(); ++i) {
    size_t j = i;
    while (j == i) { j = get_random_number(_nodes.size()); }

    if (i < j) { connections.insert(make_pair(i,j)); }
    else       { connections.insert(make_pair(j,i)); }
  }

  // Add few more connections for good measure
  for (size_t i = 0; i < _nodes.size(); ++i) {
    while (get_random_number(_nodes.size()) < _nodes.size() / 2) {
      size_t j = i;
      while (j == i) { j = get_random_number(_nodes.size()); }

      if (i < j) { connections.insert(make_pair(i,j)); }
      else       { connections.insert(make_pair(j,i)); }
    }
  }

  for (auto c : connections) {
    // Do it both ways so that we know right away who is connected
    // to whom.
    _nodes[c.first].connect(_nodes[c.second].local_endpoint());
    _nodes[c.second].connect(_nodes[c.first].local_endpoint());
  }
}

bool Graph::is_MIS() const {
  assert("TODO" && 0);
}

void Graph::shutdown() {
  for (auto& n : _nodes) {
    n.shutdown();
  }
}

std::ostream& operator<<(std::ostream& os, const Graph& g) {
  for (const auto& node : g._nodes) {
    os << node << endl;
  }
  return os;
}

