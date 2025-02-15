// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <string>
#include <vector>
#include <algorithm>

#include "hwinfo/WMIwrapper.h"
#include "hwinfo/cpu.h"

#include "hwinfo/cpuid.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
int CPU::currentClockSpeed_kHz() {
  std::vector<int64_t> speed {};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speed);
  if (speed.empty()) { return -1; }
  return speed[0];
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
  std::vector<const wchar_t*> vendor {};
  wmi::queryWMI("Win32_Processor", "Manufacturer", vendor);
  if (vendor.empty()) { return "<unknown>"; }
  std::wstring tmp(vendor[0]);
  return {tmp.begin(), tmp.end()};
#endif
  return "<unknown>";
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
  std::vector<const wchar_t*> vendor {};
  wmi::queryWMI("Win32_Processor", "Name", vendor);
  if (vendor.empty()) { return "<unknown>"; }
  std::wstring tmp(vendor[0]);
  return {tmp.begin(), tmp.end()};
#endif
  return "<unknown>";
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
        return numCores;
      }
    }
  }
  return -1;
#else
  std::vector<int> cores {};
  wmi::queryWMI("Win32_Processor", "NumberOfCores", cores);
  if (cores.empty()) { return -1; }
  return cores[0];
#endif
  return -1;
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
  std::vector<int> cores {};
  wmi::queryWMI("Win32_Processor", "NumberOfThreads", cores);
  if (cores.empty()) { return -1; }
  return cores[0];
#endif
  return -1;
}

// _____________________________________________________________________________________________________________________
int CPU::getMaxClockSpeed_kHz() {
  std::vector<int64_t> speed {};
  wmi::queryWMI("Win32_Processor", "MaxClockSpeed", speed);
  if (speed.empty()) { return -1; }
  return speed[0] * 1000;
}

// _____________________________________________________________________________________________________________________
int CPU::getRegularClockSpeed_kHz() {
  std::vector<int64_t> speed {};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speed);
  if (speed.empty()) { return -1; }
  return speed[0] * 1000;
}

int CPU::getCacheSize_Bytes() {
  std::vector<int64_t> cacheSize {};
  wmi::queryWMI("Win32_Processor", "L3CacheSize", cacheSize);
  if (cacheSize.empty()) { return -1; }
  return cacheSize[0];
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS