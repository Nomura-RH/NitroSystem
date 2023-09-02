#include "g2di_BGManipulator.h"

REGType16v * const NNSiG2dBGCNTTable[] = {
    &reg_G2_BG0CNT, &reg_G2_BG1CNT, &reg_G2_BG2CNT, &reg_G2_BG3CNT,
    &reg_G2S_DB_BG0CNT, &reg_G2S_DB_BG1CNT, &reg_G2S_DB_BG2CNT, &reg_G2S_DB_BG3CNT
};

void NNSi_G2dBGGetCharSize (int * pWidth, int * pHeight, NNSG2dBGSelect n)
{
    const int bgNo = GetBGNo(n);
    const int scnSize = (*GetBGnCNT(n) & REG_G2_BG0CNT_SCREENSIZE_MASK) >> REG_G2_BG0CNT_SCREENSIZE_SHIFT;
    const int bgMode = IsMainBG(n) ? GetBGModeMain(): GetBGModeSub();
    const BOOL bAffine = (((bgNo == 2) && ((bgMode == 2) || (bgMode >= 4))) || ((bgNo == 3) && (bgMode >= 1)));

    if (bAffine) {
        const int size = (16 << scnSize);
        *pWidth = size;
        *pHeight = size;
    } else {
        *pWidth = (scnSize & 1) ? 64: 32;
        *pHeight = (scnSize & 2) ? 64: 32;
    }
}
