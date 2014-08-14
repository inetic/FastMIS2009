
#include "Graph.h"
#include "../log.h"

using namespace std;
using Node  = Graph::Node;
using Nodes = Graph::Nodes;

static
void extract_connected_subgraph(Graph& g, Nodes::iterator i, Graph& target) {
  Node n = *i;
  g.nodes.erase(i);
  target.nodes.insert(n);

  for (ID neighbor_id : n.neighbors) {
    auto neighbor_i = g.nodes.find(Node(neighbor_id));

    // Already extracted
    if (neighbor_i == g.nodes.end()) continue;

    extract_connected_subgraph(g, neighbor_i, target);
  }
}

void Graph::connect(ID i, ID j) {
  auto ni = nodes.find(i);
  auto nj = nodes.find(j);
  assert(ni != nodes.end() && nj != nodes.end());
  const_cast<std::set<ID>&>(ni->neighbors).insert(j);
  const_cast<std::set<ID>&>(nj->neighbors).insert(i);
}

vector<Graph> Graph::connected_subgraphs() const {
  vector<Graph> result;

  Graph g = *this;

  while (!g.nodes.empty()) {
    Graph connected;
    extract_connected_subgraph(g, g.nodes.begin(), connected);
    result.push_back(connected);
  }

  return result;
}

static bool has_leader_neighbor(const Graph& g, const Node& n) {
  for (ID neighbor_id : n.neighbors) {
    auto i = g.nodes.find(neighbor_id);
    assert(i != g.nodes.end());
    if (i->leader_status == LeaderStatus::leader) {
      return true;
    }
  }
  return false;
}

bool Graph::is_MIS() const {
  for (const auto& node : nodes) {
    switch (node.leader_status) {
      case LeaderStatus::undecided:
        return false;
      case LeaderStatus::follower:
        if (!has_leader_neighbor(*this, node)) { return false; }
        break;
      case LeaderStatus::leader:
        if (has_leader_neighbor(*this, node)) return false;
        break;
    }
  }
  return true;
}

