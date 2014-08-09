#ifndef __PERIODIC_TIMER_H__
#define __PERIODIC_TIMER_H__

#include <functional>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "constants.h"

class PeriodicTimer {
  using Error = boost::system::error_code;

public:
  template<class Callback>
  PeriodicTimer(boost::asio::io_service& ios, Callback&& callback)
    : _was_destroyed(new bool(false))
    , _timer(ios)
    , _callback(callback)
  {
    auto was_destroyed = _was_destroyed;

    // Do first tick as soon as possible.
    ios.post([this, was_destroyed]() {
        if (*was_destroyed) return;
        on_timeout();
        });
  }

  ~PeriodicTimer() {
    *_was_destroyed = true;
  }

private:
  void on_timeout() {
    using namespace boost::posix_time;

    // Carefull, the callback may destroy this object.
    auto was_destroyed = _was_destroyed;
    auto callback_copy = _callback;
    callback_copy();
    if (*was_destroyed) return;

    _timer.expires_from_now(milliseconds(PING_TIMEOUT_MS));

    _timer.async_wait([this, was_destroyed](Error) {
        if (*was_destroyed) return;
        on_timeout();
        });
  }

private:
  std::shared_ptr<bool>       _was_destroyed;
  boost::asio::deadline_timer _timer;
  std::function<void()>       _callback;
};


#endif // ifndef __PERIODIC_TIMER_H__
