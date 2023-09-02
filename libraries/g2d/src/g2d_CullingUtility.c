#include <nnsys/g2d/g2d_CullingUtility.h>

BOOL NNS_G2dIsInViewCircle (const NNSG2dFVec2 * pTopLeft, const NNSG2dFVec2 * pSize, const NNSG2dFVec2 * pos, fx32 boundingR)
{
    if (pos->x < pTopLeft->x - boundingR)
        return FALSE;
    if (pos->y < pTopLeft->y - boundingR)
        return FALSE;
    if (pos->x > pTopLeft->x + pSize->x)
        return FALSE;
    if (pos->y > pTopLeft->y + pSize->y)
        return FALSE;

    return TRUE;
}
