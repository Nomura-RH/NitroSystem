#include <nitro.h>

#include <nnsys/g2d/g2d_OAMEx.h>
#include <nnsys/g2d/fmt/g2d_Cell_data.h>

#include "g2d_Internal.h"

static GXOamAttr defaultOam_ = { 193, 193, 193 };

static NNS_G2D_INLINE BOOL IsOamEntryFuncsValid_ (const NNSG2dOamManagerInstanceEx * pMan, const NNSG2dOamExEntryFunctions * pF)
{
    const BOOL bValid = (BOOL)((pF                       != NULL) &&
                               (pF->getOamCapacity       != NULL) &&
                               (pF->entryNewOam          != NULL));

    if (pMan->pAffineBuffer != NULL || pMan->lengthAffineBuffer != 0) {

        return (BOOL)(bValid &&
                      (pF->getAffineCapacity    != NULL) &&
                      (pF->entryNewAffine       != NULL));
    } else {
        return bValid;
    }

}

static NNS_G2D_INLINE NNSG2dOamChunk * GetNewOamChunk_ (NNSG2dOamManagerInstanceEx * pMan, const GXOamAttr * pOam)
{
    NNSG2dOamChunk * pRet = NULL;
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pOam);

    if (pMan->numUsedOam < pMan->numPooledOam) {
        pRet = &pMan->pPoolOamChunks[pMan->numUsedOam];
        pMan->numUsedOam++;

        pRet->oam = *pOam;
        return pRet;
    } else {
        NNSI_G2D_DEBUGMSG0("We have no capacity for a new Oam.");
        return NULL;
    }
}

static NNS_G2D_INLINE void AddFront_ (NNSG2dOamChunkList * pOamList, NNSG2dOamChunk * pChunk)
{
    pChunk->pNext = pOamList->pChunks;
    pOamList->pChunks = pChunk;

    pOamList->numChunks++;
}

static NNS_G2D_INLINE void AddBack_ (NNSG2dOamChunkList * pOamList, NNSG2dOamChunk * pChunk)
{
    pChunk->pNext = NULL;

    if (pOamList->pLastChunk != NULL) {
        pOamList->pLastChunk->pNext = pChunk;
    } else {
        pOamList->pChunks = pChunk;
    }

    pOamList->pLastChunk = pChunk;
    pOamList->numChunks++;
}

static NNS_G2D_INLINE void EntryOamToToBaseModule_ (NNSG2dOamManagerInstanceEx * pMan, const GXOamAttr * pOam, u16 totalOam)
{
    (void)(*pMan->oamEntryFuncs.entryNewOam)(pOam, totalOam);
}

static NNS_G2D_INLINE BOOL IsAffineProxyValid_ (NNSG2dOamManagerInstanceEx * pMan)
{
    return (pMan->pAffineBuffer != NULL && pMan->lengthAffineBuffer != 0) ? TRUE : FALSE;
}

static NNS_G2D_INLINE BOOL HasEnoughCapacity_ (NNSG2dOamManagerInstanceEx * pMan)
{
    NNS_G2D_NULL_ASSERT(pMan);
    return (pMan->numAffineBufferUsed < pMan->lengthAffineBuffer) ? TRUE : FALSE;
}

static NNS_G2D_INLINE u16 ConvertAffineIndex_ (NNSG2dOamManagerInstanceEx * pMan, u16 affineProxyIdx)
{
    NNS_G2D_ASSERT(IsAffineProxyValid_(pMan));
    NNS_G2D_ASSERT(affineProxyIdx < pMan->lengthAffineBuffer);

    return pMan->pAffineBuffer[affineProxyIdx].affineHWIndex;
}

static NNS_G2D_INLINE void ReindexOamChunkList_ (NNSG2dOamManagerInstanceEx * pMan, NNSG2dOamChunkList * pChunkList)
{
#ifdef NNS_G2D_OAMEX_USE_OLD_REINDEXOAMCHUNKLIST_

    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pChunkList);
    {
        NNSG2dOamChunk * pChunk = pChunkList->pAffinedChunks;
        NNSG2dOamChunk * pNextChunk = NULL;

        while (pChunk != NULL) {
            const u16 index = ConvertAffineIndex_(pMan, pChunk->affineProxyIdx);

            pNextChunk = pChunk->pNext;

            if (index != NNS_G2D_OAMEX_HW_ID_NOT_INIT) {
                pChunk->oam.rsParam = index;
                AddFront_(pChunkList, pChunk);
            }
            pChunk = pNextChunk;
        }
    }
#else

    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pChunkList);
    {
        NNSG2dOamChunk * pChunk = pChunkList->pAffinedChunks;
        NNSG2dOamChunk * pHeadChunk = NULL;
        NNSG2dOamChunk * pPrevChunk = NULL;
        int numAffinedOam = 0;

        while (pChunk != NULL) {
            const u16 index = ConvertAffineIndex_(pMan, pChunk->affineProxyIdx);

            if (index != NNS_G2D_OAMEX_HW_ID_NOT_INIT) {
                if (pHeadChunk == NULL) {
                    pHeadChunk = pChunk;
                }

                pChunk->oam.rsParam = index;

                pPrevChunk = pChunk;
                pChunk = pChunk->pNext;
                numAffinedOam++;

            } else {
                if (pPrevChunk != NULL) {
                    pPrevChunk->pNext = pChunk->pNext;
                }
                pMan->numUsedOam--;

                pChunk = pChunk->pNext;
            }
        }

        if (numAffinedOam != 0) {
            pPrevChunk->pNext = pChunkList->pChunks;
            pChunkList->pChunks = pHeadChunk;
            pChunkList->numChunks += numAffinedOam;
            pChunkList->pAffinedChunks = NULL;
        }
    }
#endif
}

static NNS_G2D_INLINE void ReindexAffinedOams_ (NNSG2dOamManagerInstanceEx * pMan)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_ASSERT(pMan->numAffineBufferUsed != 0);
    NNS_G2D_ASSERT(IsAffineProxyValid_(pMan));

    {
        u16 i;

        for (i = 0; i < pMan->lengthOfOrderingTbl; i++) {
            ReindexOamChunkList_(pMan, &pMan->pOamOrderingTbl[i]);
        }
    }
}

static NNS_G2D_INLINE BOOL IsPriorityValid_ (NNSG2dOamManagerInstanceEx * pMan, u8 priority)
{
    return (priority < pMan->lengthOfOrderingTbl) ? TRUE : FALSE;
}

static NNS_G2D_INLINE BOOL EntryNewOamWithAffine_ (NNSG2dOamChunkList * pOamList, NNSG2dOamChunk * pOamChunk, u16 affineIdx, NNSG2dOamExDrawOrder drawOrderType)
{
    NNS_G2D_NULL_ASSERT(pOamList);

    if (pOamChunk != NULL) {
        if (affineIdx == NNS_G2D_OAM_AFFINE_IDX_NONE) {
            if (drawOrderType == NNSG2D_OAMEX_DRAWORDER_BACKWARD) {
                AddFront_(pOamList, pOamChunk);
            } else {
                AddBack_(pOamList, pOamChunk);
            }
        } else {
            pOamChunk->affineProxyIdx = affineIdx;
            if (drawOrderType == NNSG2D_OAMEX_DRAWORDER_BACKWARD) {
                pOamChunk->pNext = pOamList->pAffinedChunks;
                pOamList->pAffinedChunks = pOamChunk;
            } else {
                pOamChunk->pNext = NULL;
                if (pOamList->pLastAffinedChunk != NULL) {
                    pOamList->pLastAffinedChunk->pNext = pOamChunk;
                } else {
                    pOamList->pAffinedChunks = pOamChunk;
                }
                pOamList->pLastAffinedChunk = pOamChunk;
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

static NNS_G2D_INLINE NNSG2dOamChunk * AdvancePointer_ (NNSG2dOamChunk * pChunk, u16 num)
{
    NNS_G2D_NULL_ASSERT(pChunk);
    while (num > 0) {
        pChunk = pChunk->pNext;
        num--;
        NNS_G2D_NULL_ASSERT(pChunk);
    }

    return pChunk;
}

static NNS_G2D_INLINE u16 DrawOamChunks_ (NNSG2dOamManagerInstanceEx * pMan, NNSG2dOamChunkList * pOamList, NNSG2dOamChunk * pChunk, u16 numOam, u16 capacityOfHW, u16 numTotalOamDrawn)
{
    while (numOam > 0 && (capacityOfHW - numTotalOamDrawn) > 0) {
        EntryOamToToBaseModule_(pMan, &pChunk->oam, numTotalOamDrawn);

        pChunk = pChunk->pNext;

        if (pChunk == NULL) {
            pChunk = pOamList->pChunks;
        }

        numOam--;
        numTotalOamDrawn++;
    }
    return numTotalOamDrawn;
}

static NNS_G2D_INLINE void LoadAffineProxyToBaseModule_ (NNSG2dOamManagerInstanceEx * pMan)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_ASSERT(pMan->numAffineBufferUsed != 0);

    {
        u16 count = 0x0;
        u16 i = pMan->lastFrameAffineIdx;
        const u16 numAffine = pMan->numAffineBufferUsed;
        const u16 capacity = (*pMan->oamEntryFuncs.getAffineCapacity)();

        NNSG2dAffineParamProxy * pAff = NULL;

        while ((count < numAffine) && (count < capacity)) {
            if (i >= numAffine) {
                i = 0;
            }

            pAff = &pMan->pAffineBuffer[ i ];
            pAff->affineHWIndex = (*pMan->oamEntryFuncs.entryNewAffine)(&pAff->mtxAffine, count);

            i++;
            count++;
        }

        pMan->lastFrameAffineIdx = i;
    }
}

static NNS_G2D_INLINE void CalcDrawnListArea_ (NNSG2dOamManagerInstanceEx * pMan)
{
    NNSG2dOamChunk * pChunk = NULL;
    NNSG2dOamChunkList * pOamList = NULL;
    u16 numTotalOamDrawn = 0;
    const u16 capacityOfHW = (*(pMan->oamEntryFuncs.getOamCapacity))();

    u16 i;

    for (i = 0; i < pMan->lengthOfOrderingTbl; i++) {
        pMan->pOamOrderingTbl[i].bDrawn = FALSE;
    }

    i = (u16)(pMan->lastRenderedOrderingTblIdx);

    while (numTotalOamDrawn < pMan->numUsedOam) {
        if (i >= pMan->lengthOfOrderingTbl) {
            i = 0;
        }

        pOamList = &pMan->pOamOrderingTbl[i];

        if (pOamList->numChunks != 0) {
            const u16 currentCapacity = (u16)(capacityOfHW - numTotalOamDrawn);

            pOamList->bDrawn = TRUE;
            pMan->lastRenderedOrderingTblIdx = i;

            if (pOamList->numChunks <= currentCapacity) {
                pOamList->numLastFrameDrawn = 0;
                pOamList->numDrawn = pOamList->numChunks;
            } else {
                if ((pOamList->numDrawn + pOamList->numLastFrameDrawn) / pOamList->numChunks > 0) {
                    pMan->lastRenderedOrderingTblIdx = (u16)(i + 1);
                }

                pOamList->numLastFrameDrawn = (u16)((pOamList->numDrawn +
                                                     pOamList->numLastFrameDrawn) % pOamList->numChunks);

                pOamList->numDrawn = currentCapacity;

                break;
            }
            numTotalOamDrawn += pOamList->numDrawn;
        }
        i++;
    }
}

void NNS_G2dResetOamManExBuffer (NNSG2dOamManagerInstanceEx * pOam)
{
    NNS_G2D_NULL_ASSERT(pOam);
    pOam->numUsedOam = 0;
    pOam->numAffineBufferUsed = 0;

    {
        u16 i = 0;
        for (i = 0; i < pOam->lengthOfOrderingTbl; i++) {
            pOam->pOamOrderingTbl[i].numChunks = 0;
            pOam->pOamOrderingTbl[i].pChunks = NULL;
            pOam->pOamOrderingTbl[i].pAffinedChunks = NULL;
            pOam->pOamOrderingTbl[i].pLastChunk = NULL;
            pOam->pOamOrderingTbl[i].pLastAffinedChunk = NULL;
        }
    }
}

BOOL NNS_G2dGetOamManExInstance (NNSG2dOamManagerInstanceEx * pOam, NNSG2dOamChunkList * pOamOrderingTbl, u8 lengthOfOrderingTbl, u16 numPooledOam, NNSG2dOamChunk * pPooledOam, u16 lengthAffineBuffer, NNSG2dAffineParamProxy * pAffineBuffer)
{
    NNS_G2D_NULL_ASSERT(pOam);
    NNS_G2D_ASSERT(lengthOfOrderingTbl != 0);
    NNS_G2D_NULL_ASSERT(pPooledOam);
    NNS_G2D_ASSERT(numPooledOam != 0);

    pOam->drawOrderType = NNSG2D_OAMEX_DRAWORDER_BACKWARD;

    MI_CpuClear32(pOamOrderingTbl, lengthOfOrderingTbl * sizeof(NNSG2dOamChunkList));

    pOam->pOamOrderingTbl = pOamOrderingTbl;
    pOam->lengthOfOrderingTbl = lengthOfOrderingTbl;

    pOam->numPooledOam = numPooledOam;
    pOam->pPoolOamChunks = pPooledOam;

    pOam->lengthAffineBuffer = lengthAffineBuffer;
    pOam->pAffineBuffer = pAffineBuffer;

    {
        NNSG2dOamExEntryFunctions * pFuncs = &pOam->oamEntryFuncs;

        pFuncs->getOamCapacity = NULL;
        pFuncs->getAffineCapacity = NULL;
        pFuncs->entryNewOam = NULL;
        pFuncs->entryNewAffine = NULL;
    }

    NNS_G2dResetOamManExBuffer(pOam);

    return TRUE;
}

void NNSG2d_SetOamManExDrawOrderType (NNSG2dOamManagerInstanceEx * pOam, NNSG2dOamExDrawOrder drawOrderType)
{
    NNS_G2D_NULL_ASSERT(pOam);
    pOam->drawOrderType = drawOrderType;
}

void NNS_G2dSetOamManExEntryFunctions (NNSG2dOamManagerInstanceEx * pMan, const NNSG2dOamExEntryFunctions * pSrc)
{
    NNS_G2D_NULL_ASSERT(pMan);
    {
        NNSG2dOamExEntryFunctions * pDst = &pMan->oamEntryFuncs;

        NNS_G2D_NULL_ASSERT(pSrc);

        if (pSrc->getOamCapacity != NULL) {
            pDst->getOamCapacity = pSrc->getOamCapacity;
        }

        if (pSrc->getAffineCapacity != NULL) {
            pDst->getAffineCapacity = pSrc->getAffineCapacity;
        }

        if (pSrc->entryNewOam != NULL) {
            pDst->entryNewOam = pSrc->entryNewOam;
        }

        if (pSrc->entryNewAffine != NULL) {
            pDst->entryNewAffine = pSrc->entryNewAffine;
        }

        NNS_G2D_ASSERT(IsOamEntryFuncsValid_(pMan, pDst));
    }
}

BOOL NNS_G2dEntryOamManExOam (NNSG2dOamManagerInstanceEx * pMan, const GXOamAttr * pOam, u8 priority)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pOam);
    NNS_G2D_ASSERT(IsPriorityValid_(pMan, priority));

    {
        NNSG2dOamChunkList * pOamList = &pMan->pOamOrderingTbl[ priority ];
        NNSG2dOamChunk * pOamChunk = GetNewOamChunk_(pMan, pOam);

        return EntryNewOamWithAffine_(pOamList,
                                      pOamChunk,
                                      NNS_G2D_OAM_AFFINE_IDX_NONE,
                                      pMan->drawOrderType);
    }
}

BOOL NNS_G2dEntryOamManExOamWithAffineIdx (NNSG2dOamManagerInstanceEx * pMan, const GXOamAttr * pOam, u8 priority, u16 affineIdx)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pOam);

    NNS_G2D_ASSERT(pOam->rsMode & 0x1);
    NNS_G2D_ASSERT(IsPriorityValid_(pMan, priority));

    {
        NNSG2dOamChunkList * pOamList = &pMan->pOamOrderingTbl[ priority ];
        NNSG2dOamChunk * pOamChunk = GetNewOamChunk_(pMan, pOam);

        return EntryNewOamWithAffine_(pOamList,
                                      pOamChunk,
                                      affineIdx,
                                      pMan->drawOrderType);
    }
}

u16 NNS_G2dEntryOamManExAffine (NNSG2dOamManagerInstanceEx * pMan, const MtxFx22 * mtx)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(mtx);
    NNS_G2D_ASSERT(IsAffineProxyValid_(pMan));

    if (HasEnoughCapacity_(pMan)) {
        NNSG2dAffineParamProxy * pAffineProxy = &pMan->pAffineBuffer[pMan->numAffineBufferUsed];

        pAffineProxy->mtxAffine = *mtx;
        pAffineProxy->affineHWIndex = NNS_G2D_OAMEX_HW_ID_NOT_INIT;

        return pMan->numAffineBufferUsed++;
    }

    return NNS_G2D_OAM_AFFINE_IDX_NONE;
}

void NNS_G2dApplyOamManExToBaseModule (NNSG2dOamManagerInstanceEx * pMan)
{
    NNS_G2D_NULL_ASSERT(pMan);
    NNS_G2D_NULL_ASSERT(pMan->oamEntryFuncs.getOamCapacity);
    {
        u16 numTotalOamDrawn = 0;
        const u16 capacityOfHW = (*(pMan->oamEntryFuncs.getOamCapacity))();

        if (pMan->numUsedOam != 0) {
            if (pMan->numAffineBufferUsed != 0) {
                LoadAffineProxyToBaseModule_(pMan);
                ReindexAffinedOams_(pMan);
            }

            CalcDrawnListArea_(pMan);

            {
                u16 i = 0;
                NNSG2dOamChunk * pChunk = NULL;
                NNSG2dOamChunkList * pOamList = NULL;

                for (i = 0; i < pMan->lengthOfOrderingTbl; i++) {
                    pOamList = &pMan->pOamOrderingTbl[i];

                    if (pOamList->bDrawn) {
                        NNS_G2D_ASSERT(pOamList->numLastFrameDrawn < pOamList->numChunks);

                        pChunk = AdvancePointer_(pOamList->pChunks, pOamList->numLastFrameDrawn);

                        numTotalOamDrawn = DrawOamChunks_(pMan,
                                                          pOamList,
                                                          pChunk,
                                                          pOamList->numDrawn,
                                                          capacityOfHW,
                                                          numTotalOamDrawn);
                    }
                }
            }
        }

        while (capacityOfHW > numTotalOamDrawn) {
            EntryOamToToBaseModule_(pMan, &defaultOam_, numTotalOamDrawn);
            numTotalOamDrawn++;
        }
    }
}
