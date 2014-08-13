#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>

#ifndef __NDEBUG
template<typename T>
void log(const T& arg) {
  std::cout << arg << std::endl;
}

template<typename T, typename... Ts>
void log(const T& arg, const Ts&... args) {
  std::cout << arg;
  log(args...);
}
#else
template<typename... Ts>
void log(const Ts&... args) {}
#endif

#endif // ifndef __LOG_H__