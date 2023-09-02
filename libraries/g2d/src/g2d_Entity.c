#include <nnsys/g2d/g2d_Entity.h>
#include <nnsys/g2d/g2d_CellAnimation.h>
#include <nnsys/g2d/g2d_MultiCellAnimation.h>
#include <nnsys/g2d/load/g2d_NAN_load.h>
#include "g2d_Internal.h"

static NNS_G2D_INLINE void ResetPaletteTbl_ (NNSG2dEntity * pEntity)
{
    NNS_G2D_ASSERT_ENTITY_VALID(pEntity);
    NNS_G2D_NULL_ASSERT(pEntity);

    NNS_G2dResetEntityPaletteTable(pEntity);
}

static NNS_G2D_INLINE void SetCurrentAnimation_ (NNSG2dEntity * pEntity)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_ASSERT_ENTITY_VALID(pEntity);

    {
        const NNSG2dAnimSequence * pAnimSeq = NNS_G2dGetAnimSequenceByIdx(pEntity->pAnimDataBank,
                                                                          pEntity->currentSequenceIdx);
        if ( pAnimSeq ) {
            switch ( pEntity->pEntityData->type ) {
            case NNS_G2D_ENTITYTYPE_CELL:
                NNS_G2dSetCellAnimationSequence((NNSG2dCellAnimation *)pEntity->pDrawStuff, pAnimSeq);
                break;
            case NNS_G2D_ENTITYTYPE_MULTICELL:
                NNS_G2dSetAnimSequenceToMCAnimation((NNSG2dMultiCellAnimation *)pEntity->pDrawStuff, pAnimSeq);
                break;
            default:
                NNS_G2D_ASSERT(FALSE);
            }
        } else {
            NNS_G2D_ASSERT(FALSE);
        }
    }
}

void NNS_G2dInitEntity (NNSG2dEntity * pEntity, void * pDrawStuff, const NNSG2dEntityData * pEntityData, const NNSG2dAnimBankData * pAnimDataBank)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_NULL_ASSERT(pDrawStuff);
    NNS_G2D_NULL_ASSERT(pEntityData);
    NNS_G2D_NULL_ASSERT(pAnimDataBank);

    pEntity->pDrawStuff = pDrawStuff;
    pEntity->pAnimDataBank = pAnimDataBank;
    pEntity->pEntityData = pEntityData;
    pEntity->currentSequenceIdx = 0;

    ResetPaletteTbl_(pEntity);
}

void NNS_G2dSetEntityCurrentAnimation (NNSG2dEntity * pEntity, u16 idx)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_NULL_ASSERT(pEntity->pAnimDataBank);
    NNS_G2D_NULL_ASSERT(pEntity->pEntityData);

    if ( pEntity->pEntityData->animData.numAnimSequence > idx ) {
        pEntity->currentSequenceIdx = pEntity->pEntityData->animData.pAnimSequenceIdxArray[idx];
        NNS_G2D_ASSERT(pEntity->pAnimDataBank->numSequences > pEntity->currentSequenceIdx);
        SetCurrentAnimation_(pEntity);
    } else {
        NNSI_G2D_DEBUGMSG0(FALSE, "Failure of finding animation data in NNS_G2dSetEntityCurrentAnimation()");
    }
}

void NNS_G2dSetEntityPaletteTable (NNSG2dEntity * pEntity, NNSG2dPaletteSwapTable * pPlttTbl)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_NULL_ASSERT(pPlttTbl);

    pEntity->pPaletteTbl = pPlttTbl;
}

void NNS_G2dResetEntityPaletteTable (NNSG2dEntity * pEntity)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    pEntity->pPaletteTbl = NULL;
}

BOOL NNS_G2dIsEntityPaletteTblEnable (const NNSG2dEntity * pEntity)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    return (pEntity->pPaletteTbl != NULL) ? TRUE : FALSE;
}

void NNS_G2dTickEntity (NNSG2dEntity * pEntity, fx32 dt)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_ASSERT_ENTITY_VALID(pEntity);

    switch (pEntity->pEntityData->type) {
    case NNS_G2D_ENTITYTYPE_CELL:
        NNS_G2dTickCellAnimation((NNSG2dCellAnimation *)pEntity->pDrawStuff, dt);
        break;
    case NNS_G2D_ENTITYTYPE_MULTICELL:
        NNS_G2dTickMCAnimation((NNSG2dMultiCellAnimation *)pEntity->pDrawStuff, dt);
        break;
    default:
        NNS_G2D_ASSERT(FALSE);
    }
}

void NNS_G2dSetEntityCurrentFrame (NNSG2dEntity * pEntity, u16 frameIndex)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_ASSERT_ENTITY_VALID(pEntity);

    switch (pEntity->pEntityData->type) {
    case NNS_G2D_ENTITYTYPE_CELL:
        NNS_G2dSetCellAnimationCurrentFrame((NNSG2dCellAnimation *)pEntity->pDrawStuff, frameIndex);
        break;
    case NNS_G2D_ENTITYTYPE_MULTICELL:
        NNS_G2dSetMCAnimationCurrentFrame((NNSG2dMultiCellAnimation *)pEntity->pDrawStuff, frameIndex);
        break;
    default:
        NNS_G2D_ASSERT(FALSE);
    }
}

void NNS_G2dSetEntitySpeed (NNSG2dEntity * pEntity, fx32 speed)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    NNS_G2D_ASSERT_ENTITY_VALID(pEntity);

    switch (pEntity->pEntityData->type) {
    case NNS_G2D_ENTITYTYPE_CELL:
        NNS_G2dSetCellAnimationSpeed((NNSG2dCellAnimation *)pEntity->pDrawStuff, speed);
        break;
    case NNS_G2D_ENTITYTYPE_MULTICELL:
        NNS_G2dSetMCAnimationSpeed((NNSG2dMultiCellAnimation *)pEntity->pDrawStuff, speed);
        break;
    default:
        NNS_G2D_ASSERT(FALSE);
    }
}

BOOL NNS_G2dIsEntityValid (NNSG2dEntity * pEntity)
{
    NNS_G2D_NULL_ASSERT(pEntity);

    if ((pEntity->pEntityData     != NULL) &&
        (pEntity->pDrawStuff      != NULL) &&
        (pEntity->pAnimDataBank   != NULL)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

NNSG2dAnimController * NNS_G2dGetEntityAnimCtrl (NNSG2dEntity * pEntity)
{
    NNS_G2D_NULL_ASSERT(pEntity);
    switch ( pEntity->pEntityData->type ) {
    case NNS_G2D_ENTITYTYPE_CELL:
        return NNS_G2dGetCellAnimationAnimCtrl((NNSG2dCellAnimation *)pEntity->pDrawStuff);
    case NNS_G2D_ENTITYTYPE_MULTICELL:
        return NNS_G2dGetMCAnimAnimCtrl((NNSG2dMultiCellAnimation *)pEntity->pDrawStuff);
    default:
        NNS_G2D_ASSERT(FALSE);
        return NULL;
    }
}
