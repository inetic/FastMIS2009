#include <set>
#include <iostream>
#include <boost/random/random_device.hpp>
#include "Random.h"
#include "Graph.h"
#include "WhenAll.h"
#include "../log.h"
#include "../Node.h"
#include "../Connection.h"

using namespace boost;
using namespace std;

static unsigned int get_random_number(int max) {
  if (max == 0) { return 0; }
  return Random::instance().generate_int(0, max - 1);
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
  for (size_t i = 1; i < _nodes.size(); ++i) {
    size_t j = get_random_number(i);

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

bool Graph::every_node_stopped() const {
  for (const auto& node : _nodes) {
    if (node.is_running_mis()) {
      return false;
    }
  }
  return true;
}

bool Graph::every_node_decided() const {
  for (const auto& node : _nodes) {
    if (node.leader_status() == LeaderStatus::undecided) {
      return false;
    }
  }
  return true;
}

bool Graph::every_neighbor_decided() const {
  for (const auto& node : _nodes) {
    if (!node.every_neighbor_decided()) {
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Graph& g) {
  for (auto i = g._nodes.begin(); i != g._nodes.end(); ++i) {
    os << *i;
    if (i != --g._nodes.end()) {
      os << endl;
    }
  }
  return os;
}

void Graph::start_fast_mis(const std::function<void()>& handler) {
  if (_nodes.empty()) return;

  WhenAll when_all(handler);

  for (auto& node : _nodes) {
    node.on_fast_mis_ended(when_all.make_continuation());
  }

  _nodes[0].start_fast_mis();
}

