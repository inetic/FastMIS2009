#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>

#ifndef NDEBUG
template<typename T>
void log(const T& arg) {
  std::cerr << arg << std::endl;
}

template<typename T, typename... Ts>
void log(const T& arg, const Ts&... args) {
  std::cerr << arg;
  log(args...);
}
#else
template<typename... Ts> void log(const Ts&...) {}
#endif

#endif // ifndef __LOG_H__
