#include "res_print_internal.h"

#include <nnsys/g3d/binres/res_struct.h>
#include <nnsys/g3d/binres/res_print.h>
#include <nnsys/g3d/binres/res_struct_accessor.h>

#if defined(SDK_DEBUG) || defined(NNS_FROM_TOOL)
void NNS_G3dPrintTexPatAnm (const NNSG3dResTexPatAnm * pPatAnm)
{
    NNS_G3D_NULL_ASSERT(pPatAnm);

    {
        u32 i, j;
        const u32 numEntries = pPatAnm->dict.numEntry;
        tabPlus_();
        for (i = 0; i < numEntries; i++) {
            const NNSG3dResDictTexPatAnmData * pAnmData =
                NNSi_G3dGetTexPatAnmDataByIdx(pPatAnm, i);
            {
                const NNSG3dResName * name =
                    NNS_G3dGetResNameByIdx(&pPatAnm->dict, i);
                chkDict_(&pPatAnm->dict, name, i);
                tabPrint_(); RES_PRINTF("Target material name, ");
                NNS_G3dPrintResName(name); RES_PRINTF("\n");
            }

            tabPrint_(); RES_PRINTF("# of FV keys, %d\n", pAnmData->numFV);

            {
                const NNSG3dResTexPatAnmFV * pfvArray =
                    (const NNSG3dResTexPatAnmFV *)((u8 *)pPatAnm + pAnmData->offset);

                tabPlus_();
                for (j = 0; j < pAnmData->numFV; j++) {
                    const NNSG3dResTexPatAnmFV * pTexPatAnm =
                        NNSi_G3dGetTexPatAnmFVByFVIndex(pPatAnm, i, j);
                    tabPrint_();
                    RES_PRINTF("frame, %d, ", pTexPatAnm->idxFrame);
                    {
                        const NNSG3dResName * name =
                            (const NNSG3dResName *)((u8 *)pPatAnm + pPatAnm->ofsTexName);
                        RES_PRINTF("texture, ");
                        NNS_G3dPrintResName(&name[pTexPatAnm->idTex]);
                    }
                    {
                        const NNSG3dResName * name =
                            (const NNSG3dResName *)((u8 *)pPatAnm + pPatAnm->ofsPlttName);
                        RES_PRINTF(", palette, ");
                        NNS_G3dPrintResName(&name[pTexPatAnm->idPltt]);
                    }
                    RES_PRINTF("\n");
                }
                tabMinus_();
            }
        }
        tabMinus_();
    }
}

void NNS_G3dPrintTexPatAnmSet (const NNSG3dResTexPatAnmSet * pAnmSet)
{
    NNS_G3D_NULL_ASSERT(pAnmSet);
    NNS_G3D_ASSERT(pAnmSet->header.kind == NNS_G3D_DATABLK_TEXPAT_ANM);

    {
        u8 i;
        const u8 numEntries = pAnmSet->dict.numEntry;

        tabPrint_(); RES_PRINTF("# of animations, %d\n", numEntries);
        tabPlus_();
        for (i = 0; i < numEntries; i++) {
            const NNSG3dResName * name =
                NNS_G3dGetResNameByIdx(&pAnmSet->dict, i);
            chkDict_(&pAnmSet->dict, name, i);
            tabPrint_(); RES_PRINTF("Texture pattern animation, ");
            NNS_G3dPrintResName(name); RES_PRINTF("\n");

            NNS_G3dPrintTexPatAnm(NNS_G3dGetTexPatAnmByIdx(pAnmSet, i));
        }
        tabMinus_();
    }
}

void NNS_G3dPrintNSBTP (const u8 * binFile)
{
    u32 i;
    const NNSG3dResFileHeader * header;
    u32 numBlocks;
    NNS_G3D_NULL_ASSERT(binFile);

    header = (const NNSG3dResFileHeader *)&binFile[0];
    NNS_G3D_ASSERT(header->sigVal == NNS_G3D_SIGNATURE_NSBTP);
    NNS_G3dPrintFileHeader(header);

    numBlocks = header->dataBlocks;
    for (i = 0; i < numBlocks; ++i) {
        const NNSG3dResDataBlockHeader * blk;
        blk = NNS_G3dGetDataBlockHeaderByIdx(header, i);
        NNS_G3D_NULL_ASSERT(blk);
        NNS_G3dPrintDataBlockHeader(blk);

        tabPlus_();
        {
            switch (blk->kind)
            {
            case NNS_G3D_DATABLK_TEXPAT_ANM:
                NNS_G3dPrintTexPatAnmSet((NNSG3dResTexPatAnmSet *)blk);
                break;
            default:
                tabPlus_();
                tabPrint_(); RES_PRINTF("cannot display this block\n");
                tabMinus_();
                break;
            }
        }
        tabMinus_();
    }
}
#endif
