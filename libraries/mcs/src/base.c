#if !defined(NNS_FINALROM)

#include <nitro.h>

#include <nnsys/misc.h>
#include <nnsys/mcs/baseCommon.h>
#include <nnsys/mcs/base.h>
#include <nnsys/mcs/ringBuffer.h>
#include <nnsys/mcs/config.h>

#include <isdbglib.h>

#include "baseCommoni.h"
#include "basei.h"

static const int SEND_RETRY_MAX = 10;
static const int UIC_WAIT_TIMEOUT_FRAME = 60 * 2;

typedef struct DefRecvCBInfo DefRecvCBInfo;
struct DefRecvCBInfo {
    NNSMcsRecvCBInfo cbInfo;
    NNSMcsRingBuffer ringBuf;
};

typedef struct NNSiMcsEnsWork NNSiMcsEnsWork;
struct NNSiMcsEnsWork {
    NNSMcsRingBuffer rdRb;
    NNSMcsRingBuffer wrRb;
    NNSiMcsEnsMsgBuf msgBuf;
};

static NNSiMcsWork * spMcsWork = NULL;

static NNSMcsDeviceCaps sDeviceCaps = {
    NITRODEVID_NULL,
    0x00000000,
};

static NNS_MCS_INLINE
BOOL IsInitialized ()
{
    return spMcsWork != NULL;
}

static NNS_MCS_INLINE
uint32_t RoundUp (uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static NNS_MCS_INLINE
uint32_t RoundDown (uint32_t value, uint32_t alignment)
{
    return value & ~(alignment - 1);
}

static NNS_MCS_INLINE
u32 min_u32 (u32 a, u32 b)
{
    return a < b ? a: b;
}

static NNSMcsRecvCBInfo * GetRecvCBInfo (NNSFndList * pList, uint32_t channel)
{
    NNSMcsRecvCBInfo * pInfo = NULL;

    while (NULL != (pInfo = NNS_FndGetNextListObject(pList, pInfo))) {
        if (pInfo->channel == channel) {
            return pInfo;
        }
    }

    return NULL;
}

static void CallbackRecv (u32 userData, u32 channel, const void * pRecv, uint32_t recvSize)
{
    const NNSiMcsMsg *const pMsg = (NNSiMcsMsg *)pRecv;
    const u32 dataSize = recvSize - NNSi_MCS_MSG_HEAD_SIZE;

    if (recvSize < NNSi_MCS_MSG_HEAD_SIZE
        || NNSi_MCS_MSG_HEAD_VERSION != pMsg->head.version
        ) {
        spMcsWork->bProtocolError = TRUE;
        return;
    }

    if (NNSi_MCS_SYSMSG_CHANNEL == channel) {
        u32 bConnect;

        if (dataSize == sizeof(bConnect)) {
            bConnect = *(u32 *)pMsg->data;
            spMcsWork->bHostDataRecived = (u8)bConnect;
        }
    } else {
        NNSFndList *const pList = (NNSFndList *)userData;
        NNSMcsRecvCBInfo *const pInfo = GetRecvCBInfo(pList, channel);

        if (pInfo) {
            (*pInfo->cbFunc)(pMsg->data, dataSize, pInfo->userData, pMsg->head.offset, pMsg->head.totalLen);
        }
    }
}

static void CallbackRecvDummy (uintptr_t, uint32_t, const void *, uint32_t)
{
    return;
}

static void DataRecvCallback (const void * pData, u32 dataSize, u32 userData, u32 offset, u32 totalSize)
{
    DefRecvCBInfo * cbInfo = (DefRecvCBInfo *)userData;

    if (NNS_McsGetRingBufferWritableBytes(cbInfo->ringBuf) < offset + dataSize) {

        NNS_WARNING(FALSE, "NNS mcs error : buffer is not enough. writable %d, data size %d, offset %d, total size %d\n",
                    NNS_McsGetRingBufferWritableBytes(cbInfo->ringBuf),
                    dataSize,
                    offset,
                    totalSize);
        return;
    }

    {
        BOOL ret = NNS_McsWriteRingBufferEx(
            cbInfo->ringBuf,
            pData,
            offset,
            dataSize,
            totalSize);
        NNS_ASSERT(ret);
    }
}

static NNS_MCS_INLINE NNSiMcsEnsWork * GetEnsWorkPtr ()
{
    return (NNSiMcsEnsWork *)NNSi_MCS_ENS_WRITE_RB_END;
}

static void SetMaskResource (u32 maskResource)
{
    const BOOL preIRQ = OS_DisableIrq();
    sDeviceCaps.maskResource = maskResource;
    (void)OS_RestoreIrq(preIRQ);
}

static void WaitSendData (void)
{
    NNS_McsPollingIdle();
    NNSi_McsSleep(8);
}

static void WaitDeviceEnable (void)
{
    NNS_McsPollingIdle();
    NNSi_McsSleep(16);
}

int _isdbusmgr_isresourceavailable(int);

__declspec(weak)
int _isdbusmgr_isresourceavailable (int)
{
    return FALSE;
}

static BOOL IsDebuggerPresent (void)
{
    static const int ISDRESOURCE_SW_DEBUGGER = 0x00200;
    return _isdbusmgr_isresourceavailable(ISDRESOURCE_SW_DEBUGGER);
}

static NNS_MCS_INLINE void Lock (void)
{
    if (OS_IsThreadAvailable()) {
        OS_LockMutex(&spMcsWork->mutex);
    }
}

static NNS_MCS_INLINE void Unlock (void)
{
    if (OS_IsThreadAvailable()) {
        OS_UnlockMutex(&spMcsWork->mutex);
    }
}

static BOOL OpenEmulator (NNSMcsDeviceCaps * pCaps)
{
    const NNSMcsRingBuffer rdRb = (NNSMcsRingBuffer)NNSi_MCS_ENS_READ_RB_START;
    const NNSMcsRingBuffer wrRb = (NNSMcsRingBuffer)NNSi_MCS_ENS_WRITE_RB_START;

    if (!NNS_McsCheckRingBuffer(rdRb) || !NNS_McsCheckRingBuffer(wrRb)) {
        return FALSE;
    }

    {
        NNSiMcsEnsWork * pWork = GetEnsWorkPtr();
        pWork->rdRb = rdRb;
        pWork->wrRb = wrRb;
    }

    pCaps->deviceID = 0;
    pCaps->maskResource = NITROMASK_RESOURCE_POLL;

    return TRUE;
}

static BOOL OpenISDevice (NNSMcsDeviceCaps * pCaps)
{
    enum {
        DEV_PRI_NONE,
        DEV_PRI_UNKNOWN,
        DEV_PRI_NITRODBG,
        DEV_PRI_NITROUIC,
        DEV_PRI_NITROUSB,
        DEV_PRI_MAX
    };

    int devPri = DEV_PRI_NONE;
    int devID = 0;
    const int devNum = NNS_McsGetMaxCaps();
    int findDevID = 0;
    const NITRODEVCAPS * pNITROCaps;

    for (findDevID = 0; findDevID < devNum; ++findDevID) {
        int findDevPri = DEV_PRI_NONE;
        pNITROCaps = NITROToolAPIGetDeviceCaps(findDevID);
        switch (pNITROCaps->m_nDeviceID)
        {
        case NITRODEVID_NITEMULATOR:
            if (IsDebuggerPresent()) {
                findDevPri = DEV_PRI_NITRODBG;
            } else {
                findDevPri = DEV_PRI_NITROUSB;
            }
            break;
        case NITRODEVID_NITUIC:
            findDevPri = DEV_PRI_NITROUIC;
            break;
        default:
            findDevPri = DEV_PRI_UNKNOWN;
        }

        if (devPri < findDevPri) {
            devPri = findDevPri;
            devID = findDevID;
        }
    }

    if (devPri == DEV_PRI_NONE || devPri == DEV_PRI_UNKNOWN) {
        OS_Warning("no device.\n");
        return FALSE;
    }

    pNITROCaps = NITROToolAPIGetDeviceCaps(devID);
    if (!NITROToolAPIOpen(pNITROCaps)) {
        return FALSE;
    }

    pCaps->deviceID = pNITROCaps->m_nDeviceID;
    pCaps->maskResource = pNITROCaps->m_dwMaskResource;
    spMcsWork->bLengthEnable = FALSE;

    {
        BOOL bSuccess = NITROToolAPISetReceiveStreamCallBackFunction(CallbackRecv, (u32) & spMcsWork->recvCBInfoList);
        NNS_ASSERT(bSuccess);
    }

    return TRUE;
}

static NNS_MCS_INLINE BOOL CloseEmulator (void)
{
    NNSiMcsEnsWork * pWork = GetEnsWorkPtr();
    pWork->rdRb = 0;
    pWork->wrRb = 0;

    return TRUE;
}

void NNS_McsInit (void * workMem)
{
    if (IsInitialized()) {
        return;
    }

    NNS_NULL_ASSERT(workMem);

    if (!OS_IsRunOnEmulator()) {
        NITROToolAPIInit();
    }

    spMcsWork = (NNSiMcsWork *)workMem;

    spMcsWork->bProtocolError = FALSE;
    spMcsWork->bLengthEnable = FALSE;
    spMcsWork->bHostDataRecived = FALSE;
    OS_InitMutex(&spMcsWork->mutex);
    NNS_FND_INIT_LIST(&spMcsWork->recvCBInfoList, NNSMcsRecvCBInfo, link);
}

int NNS_McsGetMaxCaps (void)
{
    int num;

    NNS_ASSERT(IsInitialized());
    Lock();

    if (OS_IsRunOnEmulator()) {
        num = 1;
    } else {
        num = NITROToolAPIGetMaxCaps();
    }

    Unlock();
    return num;
}

BOOL NNS_McsOpen (NNSMcsDeviceCaps * pCaps)
{
    BOOL bSuccess;

    NNS_ASSERT(IsInitialized());
    Lock();

    if (OS_IsRunOnEmulator()) {
        bSuccess = OpenEmulator(pCaps);
    } else {
        bSuccess = OpenISDevice(pCaps);
    }

    if (bSuccess) {
        sDeviceCaps.deviceID = pCaps->deviceID;
        SetMaskResource(pCaps->maskResource);

        spMcsWork->bHostDataRecived = FALSE;
    }

    Unlock();
    return bSuccess;
}

BOOL NNS_McsClose (void)
{
    BOOL bSuccess;

    NNS_ASSERT(IsInitialized());
    Lock();

    if (OS_IsRunOnEmulator()) {
        bSuccess = CloseEmulator();
    } else {
        bSuccess = NITROToolAPIClose();
    }

    if (bSuccess) {
        sDeviceCaps.deviceID = NITRODEVID_NULL;
        SetMaskResource(0x00000000);
    }

    Unlock();
    return bSuccess;
}

void NNS_McsRegisterRecvCallback (NNSMcsRecvCBInfo * pInfo, u16 channel, NNSMcsRecvCallback cbFunc, u32 userData)
{
    NNS_ASSERT(IsInitialized());
    Lock();

    NNS_ASSERT(NULL == GetRecvCBInfo(&spMcsWork->recvCBInfoList, channel));

    pInfo->channel = channel;
    pInfo->cbFunc = cbFunc;
    pInfo->userData = userData;

    NNS_FndAppendListObject(&spMcsWork->recvCBInfoList, pInfo);

    Unlock();
}

void NNS_McsRegisterStreamRecvBuffer (u16 channel, void * buf, u32 bufSize)
{
    uintptr_t start = (uintptr_t)buf;
    uintptr_t end = start + bufSize;
    u8 * pBBuf;
    DefRecvCBInfo * pInfo;

    NNS_ASSERT(IsInitialized());
    Lock();

    start = RoundUp(start, 4);
    end = RoundDown(end, 4);

    NNS_ASSERT(start < end);

    buf = (void *)start;
    bufSize = end - start;

    NNS_ASSERT(bufSize >= sizeof(DefRecvCBInfo) + sizeof(NNSiMcsRingBufferHeader) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));

    pBBuf = buf;
    pInfo = buf;

    pInfo->ringBuf = NNS_McsInitRingBuffer(pBBuf + sizeof(DefRecvCBInfo), bufSize - sizeof(DefRecvCBInfo));

    NNS_McsRegisterRecvCallback(
        &pInfo->cbInfo,
        channel,
        DataRecvCallback,
        (uintptr_t)pInfo);

    Unlock();
}

void NNS_McsUnregisterRecvResource (u16 channel)
{
    NNSMcsRecvCBInfo * pInfo = NULL;

    NNS_ASSERT(IsInitialized());
    Lock();

    pInfo = GetRecvCBInfo(&spMcsWork->recvCBInfoList, channel);
    NNS_ASSERT(pInfo);

    NNS_FndRemoveListObject(&spMcsWork->recvCBInfoList, pInfo);

    Unlock();
}

u32 NNS_McsGetStreamReadableSize (u16 channel)
{
    u32 size;

    NNS_ASSERT(IsInitialized());
    Lock();

    {
        DefRecvCBInfo * pCBInfo = (DefRecvCBInfo *)GetRecvCBInfo(&spMcsWork->recvCBInfoList, channel)->userData;

        NNS_ASSERT(pCBInfo);

        size = NNS_McsGetRingBufferReadableBytes(pCBInfo->ringBuf);
    }

    Unlock();
    return size;
}

u32 NNS_McsGetTotalStreamReadableSize (u16 channel)
{
    u32 size;

    NNS_ASSERT(IsInitialized());
    Lock();

    {
        DefRecvCBInfo * pCBInfo = (DefRecvCBInfo *)GetRecvCBInfo(&spMcsWork->recvCBInfoList, channel)->userData;

        NNS_ASSERT(pCBInfo);

        size = NNS_McsGetRingBufferTotalReadableBytes(pCBInfo->ringBuf);
    }

    Unlock();
    return size;
}

BOOL NNS_McsReadStream (u16 channel, void * data, u32 size, u32 * pReadSize)
{
    BOOL bSuccess;

    NNS_ASSERT(IsInitialized());
    Lock();

    {
        DefRecvCBInfo * pCBInfo = (DefRecvCBInfo *)GetRecvCBInfo(&spMcsWork->recvCBInfoList, channel)->userData;

        NNS_ASSERT(pCBInfo);

        bSuccess = NNS_McsReadRingBuffer(pCBInfo->ringBuf, data, size, pReadSize);
    }

    Unlock();
    return bSuccess;
}

BOOL NNS_McsGetStreamWritableLength (u32 * pLength)
{
    BOOL ret = FALSE;
    u32 length;

    NNS_ASSERT(IsInitialized());
    Lock();

    if (OS_IsRunOnEmulator()) {
        length = NNS_McsGetRingBufferWritableBytes(GetEnsWorkPtr()->wrRb);
        ret = TRUE;
    } else {
        u32 i;

        for (i = 0; i < UIC_WAIT_TIMEOUT_FRAME; ++i) {
            ret = 0 != NITROToolAPIStreamGetWritableLength(&length);

            if (!ret) {
                OS_Printf("NNS Mcs error: faild NITROToolAPIStreamGetWritableLength()\n");
                break;
            }

            if (spMcsWork->bLengthEnable) {
                break;
            }

            if (length > 0) {
                spMcsWork->bLengthEnable = TRUE;
                break;
            }

            WaitDeviceEnable();
        }
    }

    if (ret) {
        if (length < NNSi_MCS_MSG_HEAD_SIZE) {
            *pLength = 0;
        } else {
            *pLength = length - NNSi_MCS_MSG_HEAD_SIZE;
        }
    }

    Unlock();
    return ret;
}

BOOL NNS_McsWriteStream (u16 channel, const void * data, u32 size)
{
    const u8 *const pSrc = (const u8 *)data;
    int retryCnt = 0;
    u32 offset = 0;

    NNS_ASSERT(IsInitialized());
    Lock();

    while (offset < size) {
        const u32 restSize = size - offset;
        u32 length;

        if (!NNS_McsGetStreamWritableLength(&length)) {
            break;
        }

        if (restSize > length && length < NNSi_MCS_MSG_DATA_SIZE_MIN) {
            if (++retryCnt > SEND_RETRY_MAX) {
                NNS_WARNING(FALSE, "NNS Mcs error: send time out writable bytes %d, rest bytes %d\n", length, restSize);
                break;
            }
            WaitSendData();
        } else {
            NNSiMcsMsg * pBlock = &spMcsWork->writeBuf;

            length = min_u32(min_u32(restSize, length), NNSi_MCS_MSG_DATA_SIZE_MAX);

            pBlock->head.version = NNSi_MCS_MSG_HEAD_VERSION;
            pBlock->head.reserved = 0;
            pBlock->head.offset = offset;
            pBlock->head.totalLen = size;
            MI_CpuCopy8(pSrc + offset, pBlock->data, length);

            if (OS_IsRunOnEmulator()) {
                NNSiMcsEnsWork *const pWork = GetEnsWorkPtr();

                pWork->msgBuf.channel = channel;
                MI_CpuCopy8(pBlock, pWork->msgBuf.buf, NNSi_MCS_MSG_HEAD_SIZE + length);
                if (!NNS_McsWriteRingBuffer(
                        pWork->wrRb,
                        &pWork->msgBuf,
                        offsetof(NNSiMcsEnsMsgBuf, buf) + NNSi_MCS_MSG_HEAD_SIZE + length)
                    ) {
                    NNS_WARNING(FALSE, "NNS Mcs error: send error\n");
                    break;
                }
            } else {
                if (!NITROToolAPIWriteStream(
                        channel,
                        pBlock,
                        NNSi_MCS_MSG_HEAD_SIZE + length)
                    ) {
                    NNS_WARNING(FALSE, "NNS Mcs error: failed NITROToolAPIWriteStream()\n");
                    break;
                }
            }

            offset += length;
        }
    }

    Unlock();
    return offset == size;
}

void NNS_McsClearBuffer (void)
{
    NNSMcsRecvCBInfo * pInfo = NULL;

    NNS_ASSERT(IsInitialized());
    Lock();

    if (sDeviceCaps.maskResource & NITROMASK_RESOURCE_POLL) {
        if (OS_IsRunOnEmulator()) {
            NNSiMcsEnsWork * pWork = GetEnsWorkPtr();
            NNS_McsClearRingBuffer(pWork->rdRb);
        } else {
            BOOL bSuccess = NITROToolAPISetReceiveStreamCallBackFunction(CallbackRecvDummy, 0);
            NNS_ASSERT(bSuccess);

            NITROToolAPIPollingIdle();

            bSuccess = NITROToolAPISetReceiveStreamCallBackFunction(CallbackRecv, (u32) & spMcsWork->recvCBInfoList);
            NNS_ASSERT(bSuccess);
        }
    }

    while (NULL != (pInfo = NNS_FndGetNextListObject(&spMcsWork->recvCBInfoList, pInfo))) {
        if (pInfo->cbFunc == DataRecvCallback) {
            DefRecvCBInfo * pDefCBInfo = (DefRecvCBInfo *)pInfo->userData;
            NNS_McsClearRingBuffer(pDefCBInfo->ringBuf);
        }
    }

    Unlock();
}

BOOL NNS_McsIsServerConnect (void)
{
    BOOL bSuccess;

    NNS_ASSERT(IsInitialized());
    Lock();

    NNS_McsPollingIdle();

    bSuccess = spMcsWork->bHostDataRecived;

    Unlock();
    return bSuccess;
}

void NNS_McsPollingIdle (void)
{
    NNS_ASSERT(IsInitialized());
    Lock();

    if (sDeviceCaps.maskResource & NITROMASK_RESOURCE_POLL) {
        if (OS_IsRunOnEmulator()) {
            NNSiMcsEnsWork * pWork = GetEnsWorkPtr();
            uint32_t readableBytes;
            uint32_t readBytes;

            while (0 < (readableBytes = NNS_McsGetRingBufferReadableBytes(pWork->rdRb))) {
                NNS_ASSERT(offsetof(NNSiMcsEnsMsgBuf, buf) < readableBytes && readableBytes <= sizeof(NNSiMcsEnsMsgBuf));

                {
                    BOOL bRet = NNS_McsReadRingBuffer(pWork->rdRb, &pWork->msgBuf, readableBytes, &readBytes);
                    NNS_ASSERT(bRet);
                }

                CallbackRecv((u32) & spMcsWork->recvCBInfoList, pWork->msgBuf.channel, pWork->msgBuf.buf, readBytes - offsetof(NNSiMcsEnsMsgBuf, buf));
            }
        } else {
            NITROToolAPIPollingIdle();
        }

        if (spMcsWork->bProtocolError) {
            const u32 data = TRUE;
            u32 length;
            if (NNS_McsGetStreamWritableLength(&length)
                || sizeof(data) <= length
                ) {

                if (NNS_McsWriteStream(NNSi_MCS_SYSMSG_CHANNEL, &data, sizeof(data))) {

                    OS_Panic("mcs message version error.\n");
                }
            }
        }
    }

    Unlock();
}

void NNS_McsVBlankInterrupt (void)
{
    if (sDeviceCaps.maskResource & NITROMASK_RESOURCE_VBLANK) {
        NITROToolAPIVBlankInterrupt();
    }
}

void NNS_McsCartridgeInterrupt (void)
{
    if (sDeviceCaps.maskResource & NITROMASK_RESOURCE_CARTRIDGE) {
        NITROToolAPICartridgeInterrupt();
    }
}
#endif
