#ifndef __PERIODIC_TIMER_H__
#define __PERIODIC_TIMER_H__

#include <functional>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "constants.h"
#include "DestroyGuard.h"

class PeriodicTimer {
  using Error = boost::system::error_code;

public:
  template<class Callback>
  PeriodicTimer(boost::asio::io_service& ios, Callback&& callback)
    : _timer(ios)
    , _callback(callback)
  {
    auto destroyed = _destroy_guard.indicator();

    // Do first tick as soon as possible.
    ios.post([this, destroyed]() {
        if (destroyed) return;
        on_timeout();
        });
  }

private:
  void on_timeout() {
    using namespace boost::posix_time;

    // Carefull, the callback may destroy this object.
    auto destroyed = _destroy_guard.indicator();
    auto callback_copy = _callback;
    callback_copy();
    if (destroyed) return;

    _timer.expires_from_now(milliseconds(PING_TIMEOUT_MS));

    _timer.async_wait([this, destroyed](Error) {
        if (destroyed) return;
        on_timeout();
        });
  }

private:
  DestroyGuard                _destroy_guard;
  boost::asio::deadline_timer _timer;
  std::function<void()>       _callback;
};


#endif // ifndef __PERIODIC_TIMER_H__
