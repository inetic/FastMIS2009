#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <set>
#include <vector>
#include "../ID.h"
#include "../LeaderStatus.h"

struct Graph {
  struct Node {
    ID           id;
    std::set<ID> neighbors;
    LeaderStatus leader_status;

    Node(ID id)
      : id(id), leader_status(LeaderStatus::undecided)
    {}

    Node(ID id, LeaderStatus leader_status)
      : id(id), leader_status(leader_status)
    {}

    bool operator<(const Node& other) const { return id < other.id; }
  };

  void connect(ID, ID);
  std::vector<Graph> connected_subgraphs() const;
  bool is_MIS() const;

  using Nodes = std::set<Node>;
  Nodes nodes;
};

#endif // ifndef __GRAPH_H__
