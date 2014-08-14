#ifndef __DESTROY_GUARD_H__
#define __DESTROY_GUARD_H__

#include <memory>

class DestroyGuard {
public:
  class Indicator {
    friend class DestroyGuard;
  public:
    operator bool() const {
      return *_was_destroyed;
    }

  private:
    Indicator(const std::shared_ptr<bool>& was_destroyed)
      : _was_destroyed(was_destroyed) { }

  private:
    std::shared_ptr<bool> _was_destroyed;
  };

public:
  DestroyGuard()
    : _was_destroyed(std::make_shared<bool>(false))
  {}

  ~DestroyGuard() {
    *_was_destroyed = true;
  }

  Indicator indicator() {
    return Indicator(_was_destroyed);
  }

private:
  std::shared_ptr<bool> _was_destroyed;
};

#endif // ifndef __DESTROY_GUARD_H__

