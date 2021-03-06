#ifndef __LEADER_STATUS_H__
#define __LEADER_STATUS_H__

#include <iostream>
#include <stdexcept>

enum class LeaderStatus { undecided, follower, leader };

inline std::ostream& operator<<(std::ostream& os, LeaderStatus ls) {
  switch (ls) {
    case LeaderStatus::undecided:  os << "U"; break;
    case LeaderStatus::follower:   os << "F"; break;
    case LeaderStatus::leader:     os << "L"; break;
  }
  return os;
}

inline std::istream& operator>>(std::istream& is, LeaderStatus& ls) {
  std::string str;
  is >> str;

  if      (str == "U")  { ls = LeaderStatus::undecided; }
  else if (str == "F")  { ls = LeaderStatus::follower; }
  else if (str == "L")  { ls = LeaderStatus::leader; }
  else {
    throw std::runtime_error("unrecognized leader status");
  }

  return is;
}

#endif // ifndef __LEADER_STATUS_H__
