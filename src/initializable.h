#pragma once
#include <stdexcept>
#include <string>

class Initializable {
public:
  virtual ~Initializable() = default;

protected:
  // Throws std::runtime_error on failure
  void require_init(bool ok, const std::string &what) {
    if (!ok)
      throw std::runtime_error("Failed to initialize " + what);
  }
};
