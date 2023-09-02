#ifndef NNS_MCS_BASEI_H_
#define NNS_MCS_BASEI_H_

#include <nnsys/mcs/config.h>

#ifdef __cplusplus
extern "C" {
#endif

NNS_MCS_INLINE void NNSi_McsSleep (u32 milSec)
{
    if (OS_IsThreadAvailable() && OS_IsTickAvailable() && OS_IsAlarmAvailable()) {
        OS_Sleep(milSec);
    } else {
        OS_SpinWait((u32)(1LL * HW_CPU_CLOCK * milSec / 1000));
    }
}

#ifdef __cplusplus
}
#endif

#endif
