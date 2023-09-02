#ifndef NNS_MCS_PRINTI_H_
#define NNS_MCS_PRINTI_H_

#if defined(_MSC_VER)
    #include "mcsStdInt.h"
#else
    #include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NNSi_MCS_PRINT_CHANNEL    (uint16_t)('PR' + 0x8000)

#ifdef __cplusplus
}
#endif

#endif
