#ifndef __LEADER_STATUS_H__
#define __LEADER_STATUS_H__

enum LeaderStatus { undecided, not_leader, leader };

inline std::ostream& operator<<(std::ostream& os, LeaderStatus ls) {
  switch (ls) {
    case undecided:  os << "undecided"; break;
    case not_leader: os << "not_leader"; break;
    case leader:     os << "leader"; break;
  }
  return os;
}

inline std::istream& operator>>(std::istream& is, LeaderStatus& ls) {
  std::string str;
  is >> str;

  if      (str == "undecided")  { ls = undecided; }
  else if (str == "not_leader") { ls = not_leader; }
  else if (str == "leader")     { ls = leader; }
  else {
    throw std::runtime_error("unrecognized leader status");
  }

  return is;
}

#endif // ifndef __LEADER_STATUS_H__
