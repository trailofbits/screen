#ifndef __SCREEN_H
#define __SCREEN_H

/**
 * Macro to be used by client code to start a screened section.
 */
#define __screen_start(name) \
  __attribute__((annotate("screen_start_" #name))) int __screen_start_ ## name

/**
 * Macro to be used by client code to end a screened section.
 */
#define __screen_stop(name) \
  __attribute__((annotate("screen_stop_" #name))) int __screen_stop_ ## name

#endif // __SCREEN_H
