#ifndef __LEADER_STATUS_H__
#define __LEADER_STATUS_H__

enum class LeaderStatus { undecided, follower, leader };

inline std::ostream& operator<<(std::ostream& os, LeaderStatus ls) {
  switch (ls) {
    case LeaderStatus::undecided:  os << "undecided"; break;
    case LeaderStatus::follower:   os << "follower"; break;
    case LeaderStatus::leader:     os << "leader"; break;
  }
  return os;
}

inline std::istream& operator>>(std::istream& is, LeaderStatus& ls) {
  std::string str;
  is >> str;

  if      (str == "undecided")  { ls = LeaderStatus::undecided; }
  else if (str == "follower")   { ls = LeaderStatus::follower; }
  else if (str == "leader")     { ls = LeaderStatus::leader; }
  else {
    throw std::runtime_error("unrecognized leader status");
  }

  return is;
}

#endif // ifndef __LEADER_STATUS_H__
