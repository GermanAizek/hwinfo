// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <string>
#include <vector>
#include <algorithm>

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#include <math.h>
#include <pthread.h>

#include "hwinfo/cpu.h"
#include "hwinfo/cpuid.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
int CPU::currentClockSpeed_kHz() {
  // TODO: implement
  return -1;
}

// _____________________________________________________________________________________________________________________
std::string CPU::getVendor() {
#if defined(HWINFO_X86)
  std::string vendor;
  uint32_t regs[4] {0};
  cpuid::cpuid(0, 0, regs);
  vendor += std::string((const char *) &regs[1], 4);
  vendor += std::string((const char *) &regs[3], 4);
  vendor += std::string((const char *) &regs[2], 4);
  return vendor;
#else
  // TODO: implement
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string CPU::getModelName() {
#if defined(HWINFO_X86)
  std::string model;
  uint32_t regs[4] {};
  for (unsigned i = 0x80000002; i < 0x80000005; ++i) {
    cpuid::cpuid(i, 0, regs);
    for(auto c : std::string((const char*)&regs[0], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[1], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[2], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[3], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
  }
  return model;
#else
  char* model_2[1024];
  size_t size=sizeof(model_2);
  if (sysctlbyname("machdep.cpu.brand_string", model_2, &size, NULL, 0) < 0) {
      perror("sysctl");
  }
  return std::string(model);
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumPhysicalCores() {
#if defined(HWINFO_X86)
  uint32_t regs[4] {};
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 11) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4] {};
        cpuid::cpuid(0x0b, lvl, regs_2);
        uint32_t currLevel = (LVL_TYPE & regs_2[2]) >> 8;
        if (currLevel == 0x01) {
          int numCores = getNumLogicalCores()/static_cast<int>(LVL_CORES & regs_2[1]);
          if (numCores > 0) {
            return numCores;
          }
        }
      }
    } else {
      if (HFS >= 4) {
        uint32_t regs_3[4] {};
        cpuid::cpuid(4, 0, regs_3);
        int numCores = getNumLogicalCores()/static_cast<int>(1 + ((regs_3[0] >> 26) & 0x3f));
        if (numCores > 0) {
          return numCores;
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      uint32_t regs_4[4] {};
      cpuid::cpuid(0x80000000, 0, regs_4);
      if (regs_4[0] >= 8) {
        int numCores = 1 + (regs_4[2] & 0xff);
        if (numCores > 0) {
          return numCores;
        }
      }
    }
  }
  return -1;
#else
  int physical = 0;
    size_t physical_size = sizeof(physical);
    if (sysctlbyname("hw.physicalcpu", &physical, &physical_size, nullptr, 0) != 0) {
      return -1;
    }
    return physical;
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumLogicalCores() {
#if defined(HWINFO_X86)
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  uint32_t regs[4] {};
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 0xb) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4] {};
        cpuid::cpuid(0x0b, lvl, regs_2);
        uint32_t currLevel = (LVL_TYPE & regs_2[2]) >> 8;
        if (currLevel == 0x02) {
          return static_cast<int>(LVL_CORES & regs_2[1]);
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      cpuid::cpuid(1, 0, regs);
      return static_cast<int>(regs[1] >> 16) & 0xff;
    }
    return 1;
  }
  return -1;
#else
  int logical = 0;
  size_t logical_size = sizeof(logical);
  if (sysctlbyname("hw.logicalcpu", &logical, &logical_size, nullptr, 0) != 0) {
    return -1;
  }
  return logical;
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getMaxClockSpeed_kHz() {
  long speed = 0;
  size_t speed_size = sizeof(speed);
  if (sysctlbyname("hw.cpufrequency", &speed, &speed_size, nullptr, 0) != 0) {
    speed = -1;
  }
  return static_cast<int>(speed);
}

// _____________________________________________________________________________________________________________________
int CPU::getRegularClockSpeed_kHz() {
  uint64_t frequency = 0;
  size_t size = sizeof(frequency);
  if (sysctlbyname("hw.cpufrequency", &frequency, &size, nullptr, 0) == 0) {
    return static_cast<int>(frequency);
  }
  return -1;
}

int CPU::getCacheSize_Bytes() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return -1;
  }
  while (getline(stream, line)) {
    if (line.starts_with("cache size")) {
      try {
        stream.close();
        return std::stoi(line.substr(line.find(": ")+2, line.length()-3)) * 1000;
      } catch (std::invalid_argument &e) {
        return -1;
      }
    }
  }
  stream.close();
  return -1;
#elif defined(__APPLE__)
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  std::vector<int> cacheSize {};
  wmi::queryWMI("Win32_Processor", "L3CacheSize", cacheSize);
  if (cacheSize.empty()) { return -1; }
  return cacheSize[0];
#else
  return -1;
#endif
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE