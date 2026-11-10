#ifndef __OMV_CYCLES_H__
#define __OMV_CYCLES_H__
#include <stdint.h>
#define OMV_CYCLES_PER_MS 1000UL
#define OMV_CYCLES_PER_US 1UL
static inline void omv_cycles_init(void) {}
static inline uint32_t omv_cycles_now(void) { return 0; }
#endif
