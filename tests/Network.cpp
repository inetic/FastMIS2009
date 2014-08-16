#include <set>
#include <iostream>
#include <boost/random/random_device.hpp>
#include "Random.h"
#include "Network.h"
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

Network::Network(asio::io_service& ios)
  : _io_service(ios)
{}

void Network::add_nodes(size_t node_count) {
  for (size_t i = 0; i < node_count; ++i) {
    _nodes.push_back(new Node(_io_service));
  }
}

void Network::generate_connected(size_t node_count, float exp_neighbors) {
  add_nodes(node_count);

  if (node_count <= 1) return;

  set<pair<size_t, size_t>> connections;
  auto& random = Random::instance();

  // Generate minimal connected graph
  for (size_t i = 1; i < _nodes.size(); ++i) {
    size_t j = random.generate_int(0, i - 1);

    if (i < j) { connections.insert(make_pair(i,j)); }
    else       { connections.insert(make_pair(j,i)); }
  }

  if (exp_neighbors > 0.1) {
    // Add few more connections for good measure
    float stop_probability = 1.0 / (exp_neighbors + 1);

    for (size_t i = 0; i < _nodes.size(); ++i) {
      while (random.generate_float() >= stop_probability) {
        size_t j = i;
        while (j == i) { j = get_random_number(_nodes.size()); }

        if (i < j) { connections.insert(make_pair(i,j)); }
        else       { connections.insert(make_pair(j,i)); }
      }
    }
  }

  for (auto c : connections) {
    // Do it both ways so that we know right away who is connected
    // to whom.
    _nodes[c.first].connect(_nodes[c.second].local_endpoint());
    _nodes[c.second].connect(_nodes[c.first].local_endpoint());
  }
}

Graph Network::build_graph() const {
  Graph result;

  for (const auto& network_node : _nodes) {
    auto pair = result.nodes.emplace( network_node.id()
                                    , network_node.leader_status());
    auto& graph_node = *pair.first;

    network_node.each_connection([&](const Connection& c) {
        const_cast<std::set<ID>&>(graph_node.neighbors).insert(c.id());
        });
  }

  return result;
}

bool Network::is_MIS() const {
  vector<Graph> graphs = build_graph().connected_subgraphs();

  for (const auto& graph : graphs) {
    if (!graph.is_MIS()) { return false; }
  }

  return true;
}

void Network::shutdown() {
  for (auto& n : _nodes) {
    n.shutdown();
  }
}

bool Network::every_node_stopped() const {
  for (const auto& node : _nodes) {
    if (node.is_running_mis()) {
      return false;
    }
  }
  return true;
}

bool Network::every_node_decided() const {
  for (const auto& node : _nodes) {
    if (node.leader_status() == LeaderStatus::undecided) {
      return false;
    }
  }
  return true;
}

bool Network::every_neighbor_decided() const {
  for (const auto& node : _nodes) {
    if (!node.every_neighbor_decided()) {
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Network& g) {
  for (auto i = g._nodes.begin(); i != g._nodes.end(); ++i) {
    os << *i;
    if (i != --g._nodes.end()) {
      os << endl;
    }
  }
  return os;
}

void Network::start_fast_mis(const std::function<void()>& handler) {
  if (_nodes.empty()) return;

  _on_algorithm_completed = handler;

  WhenAll when_all(handler);

  for (auto& node : _nodes) {
    node.on_fast_mis_ended(when_all.make_continuation());
  }

  vector<Graph> graphs = build_graph().connected_subgraphs();

  for (const auto& g : graphs) {
    assert(!g.nodes.empty());
    auto first_node_id = g.nodes.begin()->id;
    auto first_node_i = find(first_node_id);

    assert(first_node_i != _nodes.end());
    first_node_i->start_fast_mis();
  }
}

void Network::start_fast_mis() {
  start_fast_mis(_on_algorithm_completed);
}

Network::Nodes::iterator Network::find(ID id) {
  return find_if( _nodes.begin(), _nodes.end()
                , [id](const Node& n) { return n.id() == id; });
}

void Network::add_random_node() {
  _nodes.push_back(new Node(_io_service));

  if (size() == 1) return;

  auto& n           = _nodes.back();
  auto& random      = Random::instance();
  size_t edge_count = random.generate_int(0, (size() - 1)/2 + 1);

  for (size_t i = 0; i < edge_count; ++i) {
    size_t m_id = random.generate_int(0, size() - 2);
    auto&  m    = _nodes[m_id];
    assert(m.id() != n.id());
    n.connect(m.local_endpoint());
    m.connect(n.local_endpoint());
  }

  vector<Graph> graphs = build_graph().connected_subgraphs();

  for (auto& graph : graphs) {
    WhenAll when_all(_on_algorithm_completed);

    for (auto& graph_node : graph.nodes) {
      auto& node = *find(graph_node.id);
      node.on_fast_mis_ended(when_all.make_continuation());
    }
  }

  n.start_fast_mis();
}

void Network::shutdown_random_node() {
  if (empty()) return;

  auto& random   = Random::instance();
  size_t pick_i  = random.generate_int(0, size() - 1);
  Node&  pick    = _nodes[pick_i];

  log("Removing ", pick.id());

  vector<Graph> graphs = build_graph().connected_subgraphs();

  WhenAll when_all(_on_algorithm_completed);

  for (auto& graph : graphs) {
    // Subgraphs which are not connected to 'pick' will not
    // re-elect.
    if (!graph.nodes.count(pick.id())) continue;

    for (auto& graph_node : graph.nodes) {
      auto& node = *find(graph_node.id);

      // The picked node will not decide.
      if (node.id() == pick.id()) continue;

      node.on_fast_mis_ended(when_all.make_continuation());
    }
  }

  pick.shutdown();
}

void Network::remove_dead_nodes() {
  _nodes.erase_if([](const Node& n) { return n.is_dead(); });
}

void Network::remove_singletons() {
  _nodes.erase_if([](const Node& n) { return n.size() == 0; });
}

void Network::set_ping_timeout(boost::posix_time::time_duration d) {
  for (auto& node : _nodes) {
    node.set_ping_timeout(d);
  }
}

void Network::set_max_missed_ping_count(unsigned int c) {
  for (auto& node : _nodes) {
    node.set_max_missed_ping_count(c);
  }
}

