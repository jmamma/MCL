#ifndef F
#define F(str) (str)
#endif

#ifdef DEBUGMODE
  // For ARM, we can use Serial for debug output
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  // If debug mode is off, these become no-ops
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

