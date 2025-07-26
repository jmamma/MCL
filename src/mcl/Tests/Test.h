#pragma once
#include <Arduino.h>
#include "platform.h"

// Base Test class for all system tests
class Test {
protected:
  volatile bool test_complete = false;
  volatile bool test_success = false;
  String test_name;

public:
  Test(const String &name) : test_name(name) {}
  virtual ~Test() {}

  // Core test interface
  virtual void setup() = 0;
  virtual void cleanup() = 0;
  virtual void run_tests() = 0;

  // Utility methods
  bool is_complete() const { return test_complete; }
  bool is_successful() const { return test_success; }
  String get_name() const { return test_name; }

protected:
  // Common test utilities
  virtual void log_test_start(const String &test_name) {
    DEBUG_PRINTLN("\n=== Starting " + test_name + " ===");
  }

  virtual void log_test_result(const String &test_name, bool success) {
    DEBUG_PRINTLN(test_name + (success ? ": PASS" : ": FAIL"));
  }
};
