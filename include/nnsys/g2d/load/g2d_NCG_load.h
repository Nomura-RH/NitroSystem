#ifndef NNS_G2D_NCG_LOAD_H_
#define NNS_G2D_NCG_LOAD_H_

#include <nitro.h>

#include <nnsys/g2d/g2d_config.h>
#include <nnsys/g2d/g2d_Data.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL NNS_G2dGetUnpackedCharacterData(void * pNcgrFile, NNSG2dCharacterData ** ppCharData);
void NNS_G2dUnpackNCG(NNSG2dCharacterData * pCharData);
BOOL NNS_G2dGetUnpackedBGCharacterData(void * pNcgrFile, NNSG2dCharacterData ** ppCharData);
void NNS_G2dUnpackBGNCG(NNSG2dCharacterData * pCharData);
BOOL NNS_G2dGetUnpackedCharacterPosInfo(void * pNcgrFile, NNSG2dCharacterPosInfo ** ppCharPosInfo);

#ifdef __SNC__
    NNS_G2D_DEBUG_FUNC_DECL_BEGIN void NNS_G2dPrintCharacterData (const NNSG2dCharacterData * p1) NNS_G2D_DEBUG_FUNC_DECL_END
    NNS_G2D_DEBUG_FUNC_DECL_BEGIN void NNS_G2dPrintCharacterPosInfo (const NNSG2dCharacterPosInfo * pPosInfo) NNS_G2D_DEBUG_FUNC_DECL_END
#else
    NNS_G2D_DEBUG_FUNC_DECL_BEGIN void NNS_G2dPrintCharacterData (const NNSG2dCharacterData *) NNS_G2D_DEBUG_FUNC_DECL_END
    NNS_G2D_DEBUG_FUNC_DECL_BEGIN void NNS_G2dPrintCharacterPosInfo (const NNSG2dCharacterPosInfo *) NNS_G2D_DEBUG_FUNC_DECL_END
#endif

#ifdef __cplusplus
}
#endif

#endif
