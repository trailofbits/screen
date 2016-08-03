
#pragma once

#define _SCREEN_PREFIX "screen_"

/**
 * Macro to be used by client code to mark a screened function.
 */
#define SCREEN(name) \
  _Pragma("GCC push") \
  _Pragma("GCC diagnostic ignored \"-Wattributes\"") \
  __attribute__((annotate(_SCREEN_PREFIX #name))) \
  _Pragma("GCC pop")

/**
 * Macro to be used by client code to start a screened section.
 */
#define SCREEN_START(name) \
  _Pragma("GCC push") \
  _Pragma("GCC diagnostic ignored \"-Wattributes\"") \
  char __screen_ ## name ## _start  \
    __attribute__((annotate(_SCREEN_PREFIX #name "_start"))); \
  (void) __screen_ ## name ## _start; \
  _Pragma("GCC pop")
  

/**
 * Macro to be used by client code to end a screened section.
 */
#define SCREEN_END(name) \
  _Pragma("GCC push") \
  _Pragma("GCC diagnostic ignored \"-Wattributes\"") \
  char __screen_ ## name ## _end \
  __attribute__((annotate(_SCREEN_PREFIX #name "_end"))); \
  (void) __screen_ ## name ## _end; \
  _Pragma("GCC pop")

