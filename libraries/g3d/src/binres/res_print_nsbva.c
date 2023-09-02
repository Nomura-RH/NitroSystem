#include "res_print_internal.h"

#include <nnsys/g3d/binres/res_struct.h>
#include <nnsys/g3d/binres/res_print.h>
#include <nnsys/g3d/binres/res_struct_accessor.h>

#if defined(SDK_DEBUG) || defined(NNS_FROM_TOOL)
void NNS_G3dPrintVisAnm (const NNSG3dResVisAnm * pResVisAnm)
{
    u16 i, j;
    NNS_G3D_NULL_ASSERT(pResVisAnm);

    tabPrint_(); RES_PRINTF("# of frames, %d\n", pResVisAnm->numFrame);
    tabPrint_(); RES_PRINTF("# of nodes, %d\n", pResVisAnm->numNode);
    tabPrint_(); RES_PRINTF("size of animation, %d bytes\n", pResVisAnm->size);

    tabPlus_();
    tabPrint_(); RES_PRINTF("    , "
                            "0         "
                            "1         "
                            "2         "
                            "3         "
                            "4         "
                            "5         \n");
    for (i = 0; i < pResVisAnm->numNode; i++) {
        tabPrint_(); RES_PRINTF(" %3d, ", i);
        for (j = 0; j < pResVisAnm->numFrame; j++) {
            if (j % 60 == 0 && j != 0) {
                RES_PRINTF("\n");
                tabPrint_();
                RES_PRINTF("    , ");
            }
            {
                u32 pos = (u32)(j * pResVisAnm->numNode + i);
                u32 n = pos >> 5;
                u32 mask = 1UL << (pos & 0x1f);

                if (pResVisAnm->visData[n] & mask)
                    RES_PRINTF("1");
                else
                    RES_PRINTF("0");
            }
        }
        RES_PRINTF("\n");
    }
    tabMinus_();
}

void NNS_G3dPrintVisAnmSet (const NNSG3dResVisAnmSet * pAnmSet)
{
    NNS_G3D_NULL_ASSERT(pAnmSet);
    NNS_G3D_ASSERT(pAnmSet->header.kind == NNS_G3D_DATABLK_VIS_ANM);
    {
        u8 i;
        tabPlus_();
        for (i = 0; i < pAnmSet->dict.numEntry; i++) {
            const NNSG3dResVisAnm * pVisAnm;
            const NNSG3dResName * name = NNS_G3dGetResNameByIdx(&pAnmSet->dict, i);
            chkDict_(&pAnmSet->dict, name, i);
            tabPrint_(); RES_PRINTF("Visibility Animation, ");
            NNS_G3dPrintResName(name); RES_PRINTF("\n");

            pVisAnm = NNS_G3dGetVisAnmByIdx(pAnmSet, i);
            NNS_G3dPrintVisAnm(pVisAnm);
        }
        tabMinus_();
    }
}

void NNS_G3dPrintNSBVA (const u8 * binFile)
{
    u32 i;
    const NNSG3dResFileHeader * header;
    u32 numBlocks;

    NNS_G3D_NULL_ASSERT(binFile);

    header = (const NNSG3dResFileHeader *)&binFile[0];
    NNS_G3D_ASSERT(header->sigVal == NNS_G3D_SIGNATURE_NSBVA);
    NNS_G3dPrintFileHeader(header);

    numBlocks = header->dataBlocks;
    for (i = 0; i < numBlocks; ++i) {
        const NNSG3dResDataBlockHeader * blk;

        blk = NNS_G3dGetDataBlockHeaderByIdx(header, i);
        NNS_G3D_NULL_ASSERT(blk);
        NNS_G3dPrintDataBlockHeader(blk);

        tabPlus_();
        switch (blk->kind) {
        case NNS_G3D_DATABLK_VIS_ANM:
            NNS_G3dPrintVisAnmSet((const NNSG3dResVisAnmSet *)blk);
            break;
        default:
            tabPlus_();
            tabPrint_(); RES_PRINTF("cannot display this block\n");
            tabMinus_();
            break;
        }
        tabMinus_();
    }
}
#endif
