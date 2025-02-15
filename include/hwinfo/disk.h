// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

class Disk {
 public:
  Disk(const std::string &vendor, const std::string &model, const std::string &serialNumber, int64_t size_Bytes);
  ~Disk() = default;

  [[nodiscard]] const std::string &vendor() const;
  [[nodiscard]] const std::string &model() const;
  [[nodiscard]] const std::string &serialNumber() const;
  [[nodiscard]] int64_t size_Bytes() const;

 private:
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  int64_t _size_Bytes = -1;
};

std::vector<Disk> getAllDisks();

}  // namespace hwinfo