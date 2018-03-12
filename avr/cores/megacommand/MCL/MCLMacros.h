/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMACROS_H__
#define MCLMACROS_H__

#ifdef DEBUG_MCL
#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINT_FN(x) ({DEBUG_PRINT("func_call: "); DEBUG_PRINTLN(__FUNCTION__);})

#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT_FN(x)
#endif

#endif /* MCL_H__ */
