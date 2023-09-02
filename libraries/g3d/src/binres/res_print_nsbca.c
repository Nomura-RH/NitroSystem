#include "res_print_internal.h"

#include <nnsys/g3d/binres/res_struct.h>
#include <nnsys/g3d/binres/res_print.h>
#include <nnsys/g3d/binres/res_struct_accessor.h>

#if defined(SDK_DEBUG) || defined(NNS_FROM_TOOL)
void NNS_G3dPrintJntAnm (const NNSG3dResJntAnm * pJntAnm)
{
    int i;

    NNS_G3D_NULL_ASSERT(pJntAnm);
    tabPlus_();
    {

        tabPrint_(); RES_PRINTF("# of frames, %d\n", pJntAnm->numFrame);
        tabPrint_(); RES_PRINTF("# of nodes, %d\n", pJntAnm->numNode);

        tabPlus_();
        for (i = 0; i < pJntAnm->numNode; i++) {
            tabPrint_(); RES_PRINTF(" %d", i);
            {
                u32 tag = NNS_G3dGetJntAnmSRTTag(pJntAnm, i)->tag;

                if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY)
                    RES_PRINTF(", IdentitySRT");
                else {
                    if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_T)
                        RES_PRINTF(", IdentityT");
                    else {
                        if (tag & NNS_G3D_JNTANM_SRTINFO_BASE_T)
                            RES_PRINTF(", UseBaseT");
                        else {
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_TX)
                                RES_PRINTF(", ConstTX");
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_TY)
                                RES_PRINTF(", ConstTY");
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_TZ)
                                RES_PRINTF(", ConstTZ");
                        }
                    }

                    if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_R)
                        RES_PRINTF(", IdentityR");
                    else {
                        if (!(tag & NNS_G3D_JNTANM_SRTINFO_BASE_R)) {
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_R)
                                RES_PRINTF(", ConstR");
                        } else
                            RES_PRINTF(", UseBaseR");
                    }

                    if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_S)
                        RES_PRINTF(", IdentityS");
                    else {
                        if (tag & NNS_G3D_JNTANM_SRTINFO_BASE_S)
                            RES_PRINTF(", UseBaseS");
                        else {
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_SX)
                                RES_PRINTF(", ConstSX");
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_SY)
                                RES_PRINTF(", ConstSY");
                            if (tag & NNS_G3D_JNTANM_SRTINFO_CONST_SZ)
                                RES_PRINTF(", ConstSZ");
                        }
                    }
                }
                RES_PRINTF("\n");
            }
        }
        tabMinus_();
    }
    tabMinus_();
}

void NNS_G3dPrintJntAnmSet (const NNSG3dResJntAnmSet * pJntAnmSet)
{
    u32 i;

    NNS_G3D_NULL_ASSERT(pJntAnmSet);

    tabPlus_();
    {
        for (i = 0; i < pJntAnmSet->dict.numEntry; i++) {
            const NNSG3dResJntAnm * pJntAnm;
            const NNSG3dResName * name = NNS_G3dGetResNameByIdx(&pJntAnmSet->dict, i);
            chkDict_(&pJntAnmSet->dict, name, i);
            tabPrint_(); RES_PRINTF("Joint Animation, ");
            NNS_G3dPrintResName(name); RES_PRINTF("\n");

            pJntAnm = NNS_G3dGetJntAnmByIdx(pJntAnmSet, (int)i);
            NNS_G3dPrintJntAnm(pJntAnm);
        }
    }
    tabMinus_();
}

void NNS_G3dPrintNSBCA (const u8 * binFile)
{
    u32 i;
    const NNSG3dResFileHeader * header;
    u32 numBlocks;
    NNS_G3D_NULL_ASSERT(binFile);

    header = (const NNSG3dResFileHeader *)&binFile[0];
    NNS_G3D_ASSERT(header->sigVal == NNS_G3D_SIGNATURE_NSBCA);
    NNS_G3dPrintFileHeader(header);

    numBlocks = header->dataBlocks;
    for (i = 0; i < numBlocks; ++i) {
        const NNSG3dResDataBlockHeader * blk =
            NNS_G3dGetDataBlockHeaderByIdx(header, i);
        NNS_G3dPrintDataBlockHeader(blk);

        tabPlus_();
        {
            switch (blk->kind)
            {
            case NNS_G3D_DATABLK_JNT_ANM:
                NNS_G3dPrintJntAnmSet((const NNSG3dResJntAnmSet *)blk);
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
