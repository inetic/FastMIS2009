#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

struct Ping {
  static std::string label() { return "ping"; }

  Ping() {}
  Ping(std::ostream&) {}
};

inline std::ostream& operator<<(std::ostream& os, const Ping&) {
  return os;
} 


#endif // ifndef __PROTOCOL_H__
