#include <nnsys/g2d/g2d_Screen.h>
#include <nnsys/g2d/g2d_config.h>

#include "g2di_Dma.h"
#include "g2di_BGManipulator.h"

#define BG_MODE_WARNING 8

#define NNS_G2D_BGEXTPLTT_SLOTSIZE  0x2000

#define PLANE_WIDTH     32
#define PLANE_HEIGHT    32

typedef enum NNSG2dBGExtPlttSlot {
    NNS_G2D_BGEXTPLTTSLOT_MAIN0,
    NNS_G2D_BGEXTPLTTSLOT_MAIN1,
    NNS_G2D_BGEXTPLTTSLOT_MAIN2,
    NNS_G2D_BGEXTPLTTSLOT_MAIN3,
    NNS_G2D_BGEXTPLTTSLOT_SUB0,
    NNS_G2D_BGEXTPLTTSLOT_SUB1,
    NNS_G2D_BGEXTPLTTSLOT_SUB2,
    NNS_G2D_BGEXTPLTTSLOT_SUB3
} NNSG2dBGExtPlttSlot;

typedef struct ScreenSizeMap {
    u16 width;
    u16 height;
    u16 scnSize;
} ScreenSizeMap;

static const ScreenSizeMap sTextScnSize[4] = {
    {256, 256, GX_BG_SCRSIZE_TEXT_256x256},
    {256, 512, GX_BG_SCRSIZE_TEXT_256x512},
    {512, 256, GX_BG_SCRSIZE_TEXT_512x256},
    {512, 512, GX_BG_SCRSIZE_TEXT_512x512},
};

static const ScreenSizeMap sAffineScnSize[4] = {
    {128, 128, GX_BG_SCRSIZE_AFFINE_128x128},
    {256, 256, GX_BG_SCRSIZE_AFFINE_256x256},
    {512, 512, GX_BG_SCRSIZE_AFFINE_512x512},
    {1024, 1024, GX_BG_SCRSIZE_AFFINE_1024x1024},
};

static const ScreenSizeMap sAffineExtScnSize[4] = {
    {128, 128, GX_BG_SCRSIZE_256x16PLTT_128x128},
    {256, 256, GX_BG_SCRSIZE_256x16PLTT_256x256},
    {512, 512, GX_BG_SCRSIZE_256x16PLTT_512x512},
    {1024, 1024, GX_BG_SCRSIZE_256x16PLTT_1024x1024},
};

static const u8 sBGTextModeTable[4][8] = {
    {
        GX_BGMODE_0,
        GX_BGMODE_1,
        GX_BGMODE_2,
        GX_BGMODE_3,
        GX_BGMODE_4,
        GX_BGMODE_5,
        GX_BGMODE_6,
        BG_MODE_WARNING,
    },
    {
        GX_BGMODE_0,
        GX_BGMODE_1,
        GX_BGMODE_2,
        GX_BGMODE_3,
        GX_BGMODE_4,
        GX_BGMODE_5,
        GX_BGMODE_6,
        BG_MODE_WARNING,
    },
    {
        GX_BGMODE_0,
        GX_BGMODE_1,
        GX_BGMODE_1,
        GX_BGMODE_3,
        GX_BGMODE_3,
        GX_BGMODE_3,
        GX_BGMODE_0,
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
    },
    {
        GX_BGMODE_0,
        GX_BGMODE_0,
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
        GX_BGMODE_0,
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_0 + BG_MODE_WARNING),
    }
};

static const u8 sBGAffineModeTable[2][8] = {
    {
        (GXBGMode)(GX_BGMODE_2 + BG_MODE_WARNING),
        GX_BGMODE_2,
        GX_BGMODE_2,
        GX_BGMODE_4,
        GX_BGMODE_4,
        GX_BGMODE_4,
        GX_BGMODE_4,
        (GXBGMode)(GX_BGMODE_2 + BG_MODE_WARNING),
    },
    {
        GX_BGMODE_1,
        GX_BGMODE_1,
        GX_BGMODE_2,
        GX_BGMODE_1,
        GX_BGMODE_2,
        (GXBGMode)(GX_BGMODE_1 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_1 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_1 + BG_MODE_WARNING),
    }
};
static const u8 sBG256x16PlttModeTable[2][8] = {
    {
        (GXBGMode)(GX_BGMODE_5 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_5 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_5 + BG_MODE_WARNING),
        GX_BGMODE_5,
        GX_BGMODE_5,
        GX_BGMODE_5,
        GX_BGMODE_5,
        (GXBGMode)(GX_BGMODE_5 + BG_MODE_WARNING),
    },
    {
        GX_BGMODE_3,
        GX_BGMODE_3,
        GX_BGMODE_4,
        GX_BGMODE_3,
        GX_BGMODE_4,
        GX_BGMODE_5,
        (GXBGMode)(GX_BGMODE_3 + BG_MODE_WARNING),
        (GXBGMode)(GX_BGMODE_3 + BG_MODE_WARNING),
    }
};

GXBGAreaOver NNSi_G2dBGAreaOver = GX_BG_AREAOVER_XLU;

static NNS_G2D_INLINE BOOL IsMainBGExtPlttSlot (NNSG2dBGExtPlttSlot slot)
{
    return (slot <= NNS_G2D_BGEXTPLTTSLOT_MAIN3);
}

static NNS_G2D_INLINE int CalcTextScreenOffset (int x, int y, int w, int h)
{
    const int x_blk = x / PLANE_WIDTH;
    const int y_blk = y / PLANE_HEIGHT;
    const int x_char = x % PLANE_WIDTH;
    const int y_char = y % PLANE_HEIGHT;
    const int w_blk = w / PLANE_WIDTH;
    const int h_blk = h / PLANE_WIDTH;
    const int blk_w = (x_blk == w_blk) ? (w % PLANE_WIDTH):  PLANE_WIDTH;
    const int blk_h = (y_blk == h_blk) ? (h % PLANE_HEIGHT): PLANE_HEIGHT;

    return
        w * PLANE_HEIGHT * y_blk
        + PLANE_WIDTH * blk_h * x_blk
        + blk_w * y_char
        + x_char;
}

static NNS_G2D_INLINE u32 GetCompressedPlttOriginalIndex (const NNSG2dPaletteCompressInfo * pCmpInfo, int idx)
{
    NNS_G2D_POINTER_ASSERT(pCmpInfo);
    return ((u16 *)(pCmpInfo->pPlttIdxTbl))[idx];
}

static NNS_G2D_INLINE u32 GetPlttSize (const NNSG2dPaletteData * pPltData)
{
    NNS_G2D_POINTER_ASSERT(pPltData);

    switch (pPltData->fmt) {
    case GX_TEXFMT_PLTT16:
        return sizeof(GXBGPltt16);
    case GX_TEXFMT_PLTT256:
        return sizeof(GXBGPltt256);
    default:
        SDK_ASSERTMSG(FALSE, "invalid NNSG2dPaletteData");
    }

    return 0;
}

static const ScreenSizeMap * SelectScnSize (const ScreenSizeMap tbl[4], int w, int h)
{
    int i;
    SDK_NULL_ASSERT(tbl);

    for (i = 0; i < 4; i++) {
        if (w <= tbl[i].width && h <= tbl[i].height) {
            return &tbl[i];
        }
    }
    return &tbl[3];
}

static void ChangeBGModeByTableMain (const u8 modeTable[])
{
    GXBGMode mode = (GXBGMode)modeTable[GetBGModeMain()];
    GXBG0As bg0as = IsBG03D() ? GX_BG0_AS_3D: GX_BG0_AS_2D;
    SDK_NULL_ASSERT(modeTable);

    if (mode >= BG_MODE_WARNING) {
        mode -= BG_MODE_WARNING;
        OS_Warning("Dangerous Main BG mode change: %d => %d", GetBGModeMain(), mode);
    }

    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, mode, bg0as);
}

static void ChangeBGModeByTableSub (const u8 modeTable[])
{
    GXBGMode mode = (GXBGMode)modeTable[GetBGModeSub()];
    SDK_NULL_ASSERT(modeTable);

    if (mode >= BG_MODE_WARNING) {
        mode -= BG_MODE_WARNING;
        OS_Warning("Dangerous Sub BG mode change: %d => %d", GetBGModeSub(), mode);
    }

    GXS_SetGraphicsMode(mode);
}

static void LoadBGPlttToExtendedPltt (NNSG2dBGExtPlttSlot slot, const NNSG2dPaletteData * pPltData, const NNSG2dPaletteCompressInfo * pCmpInfo)
{
    void (*prepairLoad)(void);
    void (*cleanupLoad)(void);
    void (*loader)(const void *, u32, u32);
    u32 offset = (u32)(slot * NNS_G2D_BGEXTPLTT_SLOTSIZE);
    SDK_NULL_ASSERT(pPltData);

    DC_FlushRange(pPltData->pRawData, pPltData->szByte);

    if (IsMainBGExtPlttSlot(slot)) {
        prepairLoad = GX_BeginLoadBGExtPltt;
        cleanupLoad = GX_EndLoadBGExtPltt;
        loader = GX_LoadBGExtPltt;
    } else {
        offset -= NNS_G2D_BGEXTPLTT_SLOTSIZE * NNS_G2D_BGEXTPLTTSLOT_SUB0;

        prepairLoad = GXS_BeginLoadBGExtPltt;
        cleanupLoad = GXS_EndLoadBGExtPltt;
        loader = GXS_LoadBGExtPltt;
    }

    if (pCmpInfo != NULL) {
        const u32 szOnePltt = GetPlttSize(pPltData);
        const int numIdx = pCmpInfo->numPalette;
        int i;

        for (i = 0; i < numIdx; i++) {
            const u32 offsetAddr = GetCompressedPlttOriginalIndex(pCmpInfo, i) * szOnePltt;
            const void * pSrc = (u8 *)pPltData->pRawData + szOnePltt * i;

            prepairLoad();
            loader(pSrc, offset + offsetAddr, szOnePltt);
            cleanupLoad();
        }
    } else {
        prepairLoad();
        loader(pPltData->pRawData, offset, pPltData->szByte);
        cleanupLoad();
    }
}

static void LoadBGPlttToNormalPltt (NNSG2dBGSelect bg, const NNSG2dPaletteData * pPltData, const NNSG2dPaletteCompressInfo * pCmpInfo)
{
    u8 * pDst;
    SDK_NULL_ASSERT(pPltData);

    DC_FlushRange(pPltData->pRawData, pPltData->szByte);

    pDst = (u8 *)(IsMainBG(bg) ? HW_BG_PLTT: HW_DB_BG_PLTT);

    if (pCmpInfo != NULL) {
        const u32 szOnePltt = GetPlttSize(pPltData);
        const int numIdx = pCmpInfo->numPalette;
        int i;

        for (i = 0; i < numIdx; i++) {
            const u32 offsetAddr = GetCompressedPlttOriginalIndex(pCmpInfo, i) * szOnePltt;
            const void * pSrc = (u8 *)pPltData->pRawData + szOnePltt * i;

            NNSi_G2dDmaCopy16(GXi_DmaId, pSrc, pDst + offsetAddr, szOnePltt);
        }
    } else {
        NNSi_G2dDmaCopy16(GXi_DmaId, pPltData->pRawData, pDst, pPltData->szByte);
    }
}

static NNSG2dBGExtPlttSlot GetBGExtPlttSlot (NNSG2dBGSelect bg)
{
    static const u16 addrTable[] = {
        REG_BG0CNT_OFFSET,
        REG_BG1CNT_OFFSET,
        0,
        0,
        REG_DB_BG0CNT_OFFSET,
        REG_DB_BG1CNT_OFFSET,
        0,
        0
    };
    u32 addr;
    NNSG2dBGExtPlttSlot slot = (NNSG2dBGExtPlttSlot)bg;

    NNS_G2D_BG_ASSERT(bg);
    addr = addrTable[bg];

    if (addr != 0) {
        addr += HW_REG_BASE;

        if ((*(u16 *)addr & REG_G2_BG0CNT_BGPLTTSLOT_MASK) != 0) {
            slot += 2;
        }
    }

    return slot;
}

static void SetBGnControlToText (NNSG2dBGSelect n, GXBGScrSizeText size, GXBGColorMode cmode, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    const int bgNo = GetBGNo(n);
    GXBGExtPltt extPltt = GX_BG_EXTPLTT_01;
    if (IsMainBG(n)) {
        if (!IsMainBGExtPltt01Available()) extPltt = GX_BG_EXTPLTT_23;
        ChangeBGModeByTableMain(sBGTextModeTable[bgNo]);
    } else {
        ChangeBGModeByTableSub(sBGTextModeTable[bgNo]);
    }
    SetBGnControlText(n, size, cmode, scnBase, chrBase, extPltt);
}
static void SetBGnControlToAffine (NNSG2dBGSelect n, GXBGScrSizeAffine size, GXBGAreaOver areaOver, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    if (IsMainBG(n)) {
        ChangeBGModeByTableMain(sBGAffineModeTable[n - 2]);
    } else {
        ChangeBGModeByTableSub(sBGAffineModeTable[n - 6]);
    }
    SetBGnControlAffine(n, size, areaOver, scnBase, chrBase);
}
static void SetBGnControlTo256x16Pltt (NNSG2dBGSelect n, GXBGScrSize256x16Pltt size, GXBGAreaOver areaOver, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    if (IsMainBG(n)) {
        ChangeBGModeByTableMain(sBG256x16PlttModeTable[n - 2]);
    } else {
        ChangeBGModeByTableSub(sBG256x16PlttModeTable[n - 6]);
    }
    SetBGnControl256x16Pltt(n, size, areaOver, scnBase, chrBase);
}

static void SetBGControlText (NNSG2dBGSelect bg, GXBGColorMode colorMode, int screenWidth, int screenHeight, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    const ScreenSizeMap * pSizeMap;

    NNS_G2D_BG_ASSERT(bg);

    pSizeMap = SelectScnSize(sTextScnSize, screenWidth, screenHeight);
    SDK_ASSERTMSG(pSizeMap != NULL,
                  "Unsupported screen size(%dx%d) in input screen data",
                  screenWidth,
                  screenHeight
                  );

    SetBGnControlToText(bg, (GXBGScrSizeText)pSizeMap->scnSize, colorMode, scnBase, chrBase);
}

static void SetBGControlAffine (NNSG2dBGSelect bg, int screenWidth, int screenHeight, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    const ScreenSizeMap * pSizeMap;

    NNS_G2D_BG_ASSERT(bg);
    SDK_ASSERTMSG((bg % 4) >= 2,
                  "You can not show affine BG on %s BG %d",
                  (IsMainBG(bg) ? "Main": "Sub"),
                  (IsMainBG(bg) ? bg: bg - NNS_G2D_BGSELECT_SUB0));

    pSizeMap = SelectScnSize(sAffineScnSize, screenWidth, screenHeight);
    SDK_ASSERTMSG(pSizeMap != NULL,
                  "Unsupported screen size(%dx%d) in input screen data",
                  screenWidth,
                  screenHeight
                  );

    SetBGnControlToAffine(bg, (GXBGScrSizeAffine)pSizeMap->scnSize, NNSi_G2dBGAreaOver, scnBase, chrBase);
}

static void SetBGControl256x16Pltt (NNSG2dBGSelect bg, int screenWidth, int screenHeight, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    const ScreenSizeMap * pSizeMap;

    NNS_G2D_BG_ASSERT(bg);
    SDK_ASSERTMSG((bg % 4) >= 2,
                  "You can not show affine BG on %s BG %d",
                  (IsMainBG(bg) ? "Main": "Sub"),
                  (IsMainBG(bg) ? bg: bg - NNS_G2D_BGSELECT_SUB0));

    pSizeMap = SelectScnSize(sAffineExtScnSize, screenWidth, screenHeight);
    SDK_ASSERTMSG(pSizeMap != NULL,
                  "Unsupported screen size(%dx%d) in input screen data",
                  screenWidth,
                  screenHeight
                  );

    SetBGnControlTo256x16Pltt(bg, (GXBGScrSize256x16Pltt)pSizeMap->scnSize, NNSi_G2dBGAreaOver, scnBase, chrBase);
}

static BOOL IsBGExtPlttSlotAssigned (NNSG2dBGExtPlttSlot slot)
{
    if (IsMainBGExtPlttSlot(slot)) {
        if (slot <= NNS_G2D_BGEXTPLTTSLOT_MAIN1) {
            return IsMainBGExtPltt01Available();
        } else {
            return IsMainBGExtPltt23Available();
        }
    } else {
        return IsSubBGExtPlttAvailable();
    }
}

static void LoadBGPaletteSelect (NNSG2dBGSelect bg, BOOL bToExtPltt, const NNSG2dPaletteData * pPltData, const NNSG2dPaletteCompressInfo * pCmpInfo)
{
    NNS_G2D_BG_ASSERT(bg);
    SDK_NULL_ASSERT(pPltData);

    if (bToExtPltt) {
        NNSG2dBGExtPlttSlot slot = GetBGExtPlttSlot(bg);
        SDK_ASSERT(IsBGExtPlttSlotAssigned(slot));
        LoadBGPlttToExtendedPltt(slot, pPltData, pCmpInfo);
    } else {
        LoadBGPlttToNormalPltt(bg, pPltData, pCmpInfo);
    }
}

static void LoadBGPalette (NNSG2dBGSelect bg, const NNSG2dPaletteData * pPltData, const NNSG2dScreenData * pScnData, const NNSG2dPaletteCompressInfo * pCmpInfo)
{
    NNS_G2D_BG_ASSERT(bg);
    SDK_NULL_ASSERT(pPltData);
    SDK_NULL_ASSERT(pScnData);

    if ((pScnData->screenFormat == NNS_G2D_SCREENFORMAT_TEXT)
        && (pScnData->colorMode == NNS_G2D_SCREENCOLORMODE_256x1)) {
        LoadBGPaletteSelect(bg, IsBGUseExtPltt(bg), pPltData, pCmpInfo);
    } else {
        BOOL bExtPlttData = !(
            (pPltData->fmt == GX_TEXFMT_PLTT16)
            || (pScnData->screenFormat == NNS_G2D_SCREENFORMAT_AFFINE)
            );

        SDK_ASSERTMSG(!bExtPlttData || IsBGUseExtPltt(bg),
                      "Input screen data requires extended BG palette, but unavailable");
        LoadBGPaletteSelect(bg, bExtPlttData, pPltData, pCmpInfo);
    }
}

static void LoadBGCharacter (NNSG2dBGSelect bg, const NNSG2dCharacterData * pChrData, const NNSG2dCharacterPosInfo * pPosInfo)
{
    u32 offset = 0;

    NNS_G2D_BG_ASSERT(bg);
    SDK_NULL_ASSERT(pChrData);
    SDK_ASSERT(pPosInfo == NULL || pPosInfo->srcPosX == 0);

    if (pPosInfo != NULL) {
        int offsetChars = pPosInfo->srcPosY * pPosInfo->srcW;
        u32 szChar = (pChrData->pixelFmt == GX_TEXFMT_PLTT256) ?
                     sizeof(GXCharFmt256): sizeof(GXCharFmt16);

        offset = offsetChars * szChar;
    }

    DC_FlushRange(pChrData->pRawData, pChrData->szByte);
    LoadBGnChar(bg, pChrData->pRawData, offset, pChrData->szByte);
}

static void LoadBGScreen (NNSG2dBGSelect bg, const NNSG2dScreenData * pScnData)
{
    GXScrFmtText * pDstBase;
    int plane_cwidth;
    int plane_cheight;
    int load_cwidth;
    int load_cheight;

    NNS_G2D_BG_ASSERT(bg);
    SDK_NULL_ASSERT(pScnData);

    pDstBase = (GXScrFmtText *)GetBGnScrPtr(bg);

    {
        const int scn_cwidth = pScnData->screenWidth / 8;
        const int scn_cheight = pScnData->screenHeight / 8;

        NNSi_G2dBGGetCharSize(&plane_cwidth, &plane_cheight, bg);
        load_cwidth = (plane_cwidth > scn_cwidth) ? scn_cwidth: plane_cwidth;
        load_cheight = (plane_cheight > scn_cheight) ? scn_cheight: plane_cheight;
    }

    DC_FlushRange((void *)pScnData->rawData, pScnData->szByte);
    NNS_G2dBGLoadScreenRect(
        pDstBase,
        pScnData,
        0, 0,
        0, 0,
        plane_cwidth, plane_cheight,
        load_cwidth, load_cheight
        );
}

static void SetBGControlAuto (NNSG2dBGSelect bg, NNSG2dScreenFormat screenFormat, GXBGColorMode colorMode, int screenWidth, int screenHeight, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    switch (screenFormat) {
    case NNS_G2D_SCREENFORMAT_TEXT:
        SetBGControlText(bg, colorMode, screenWidth, screenHeight, scnBase, chrBase);
        break;
    case NNS_G2D_SCREENFORMAT_AFFINE:
        SetBGControlAffine(bg, screenWidth, screenHeight, scnBase, chrBase);
        break;
    case NNS_G2D_SCREENFORMAT_AFFINEEXT:
        SetBGControl256x16Pltt(bg, screenWidth, screenHeight, scnBase, chrBase);
        break;
    default:
        SDK_ASSERTMSG(FALSE, "TEXT, AFFINE, and 256x16 format support only");
        break;
    }
}

static void LoadScreenPartText (void * pScreenDst, const NNSG2dScreenData * pScnData, int srcX, int srcY, int dstX, int dstY, int dstW, int dstH, int width, int height)
{
    NNS_G2D_POINTER_ASSERT(pScreenDst);
    NNS_G2D_POINTER_ASSERT(pScnData);

    {
        const int src_x_end = srcX + width;
        const int src_y_end = srcY + height;
        const int src_next_offset = pScnData->screenWidth - width;
        const int dst_next_offset = dstW - width;
        const u32 szLine = sizeof(GXScrFmtText) * width;
        const int srcW = pScnData->screenWidth / 8;
        const int srcH = pScnData->screenHeight / 8;
        const GXScrFmtText * pSrcBase = (const GXScrFmtText *)pScnData->rawData;
        GXScrFmtText * pDstBase = (GXScrFmtText *)pScreenDst;
        int sx, sy;
        int dx, dy;

        for (sy = srcY, dy = dstY; sy < src_y_end; sy++, dy++) {
            for (sx = srcX, dx = dstX; sx < src_x_end; sx++, dx++) {
                const GXScrFmtText * pSrc = pSrcBase + CalcTextScreenOffset(sx, sy, srcW, srcH);
                GXScrFmtText * pDst = pDstBase + CalcTextScreenOffset(dx, dy, dstW, dstH);
                *pDst = *pSrc;
            }
        }
    }
}

static void LoadScreenPartAffine (void * pScreenDst, const NNSG2dScreenData * pScnData, int srcX, int srcY, int dstX, int dstY, int dstW, int width, int height)
{
    NNS_G2D_POINTER_ASSERT(pScreenDst);
    NNS_G2D_POINTER_ASSERT(pScnData);

    {
        const int src_y_end = srcY + height;
        const int srcW = pScnData->screenWidth / 8;
        const u32 szLine = sizeof(GXScrFmtAffine) * width;
        const GXScrFmtAffine * pSrc = (const GXScrFmtAffine *)pScnData->rawData;
        GXScrFmtAffine * pDst = (GXScrFmtAffine *)pScreenDst;
        int y;

        pSrc += srcY * srcW + srcX;
        pDst += dstY * dstW + dstX;

        for (y = srcY; y < src_y_end; y++) {
            MI_CpuCopy8(pSrc, pDst, szLine);
            pSrc += srcW;
            pDst += dstW;
        }
    }
}

static void LoadScreenPart256x16Pltt (void * pScreenDst, const NNSG2dScreenData * pScnData, int srcX, int srcY, int dstX, int dstY, int dstW, int width, int height)
{
    NNS_G2D_POINTER_ASSERT(pScreenDst);
    NNS_G2D_POINTER_ASSERT(pScnData);

    {
        const int src_y_end = srcY + height;
        const int srcW = pScnData->screenWidth / 8;
        const u32 szLine = sizeof(GXScrFmtText) * width;
        const GXScrFmtText * pSrc = (const GXScrFmtText *)pScnData->rawData;
        GXScrFmtText * pDst = (GXScrFmtText *)pScreenDst;
        int y;

        pSrc += srcY * srcW + srcX;
        pDst += dstY * dstW + dstX;

        for (y = srcY; y < src_y_end; y++) {
            MI_CpuCopy16(pSrc, pDst, szLine);
            pSrc += srcW;
            pDst += dstW;
        }
    }
}

void NNS_G2dBGLoadElementsEx (NNSG2dBGSelect bg, const NNSG2dScreenData * pScnData, const NNSG2dCharacterData * pChrData, const NNSG2dPaletteData * pPltData, const NNSG2dCharacterPosInfo * pPosInfo, const NNSG2dPaletteCompressInfo * pCmpInfo)
{
    NNS_G2D_BG_ASSERT(bg);
    NNS_G2D_ASSERT(pPltData == NULL || pScnData != NULL);
    NNS_G2D_ASSERT(pPosInfo == NULL || pChrData != NULL);
    NNS_G2D_ASSERT(pCmpInfo == NULL || pPltData != NULL);

    NNS_G2D_ASSERTMSG(pChrData == NULL || pScnData == NULL
                      || ((pChrData->pixelFmt == GX_TEXFMT_PLTT16)
                          == (pScnData->colorMode == NNS_G2D_SCREENCOLORMODE_16x16)),
                      "Color mode mismatch between character data and screen data");
    NNS_G2D_ASSERTMSG(pPltData == NULL
                      || ((pPltData->fmt == GX_TEXFMT_PLTT16)
                          == (pScnData->colorMode == NNS_G2D_SCREENCOLORMODE_16x16)),
                      "Color mode mismatch between palette data and screen data");
    NNS_G2D_ASSERT(pChrData == NULL
                   || (NNSi_G2dGetCharacterFmtType(pChrData->characterFmt)
                       == NNS_G2D_CHARACTER_FMT_CHAR));

    if (pPltData != NULL && pScnData != NULL) {
        LoadBGPalette(bg, pPltData, pScnData, pCmpInfo);
    }
    if (pChrData != NULL) {
        LoadBGCharacter(bg, pChrData, pPosInfo);
    }
    if (pScnData != NULL) {
        LoadBGScreen(bg, pScnData);
    }
}

void NNS_G2dBGSetupEx (NNSG2dBGSelect bg, const NNSG2dScreenData * pScnData, const NNSG2dCharacterData * pChrData, const NNSG2dPaletteData * pPltData, const NNSG2dCharacterPosInfo * pPosInfo, const NNSG2dPaletteCompressInfo * pCmpInfo, GXBGScrBase scnBase, GXBGCharBase chrBase)
{
    NNS_G2D_BG_ASSERT(bg);
    NNS_G2D_NULL_ASSERT(pScnData);
    NNS_G2D_ASSERT(pPosInfo == NULL || pChrData != NULL);
    NNS_G2D_ASSERT(pCmpInfo == NULL || pPltData != NULL);

    NNS_G2D_ASSERTMSG(pChrData == NULL
                      || ((pChrData->pixelFmt == GX_TEXFMT_PLTT16)
                          == (pScnData->colorMode == NNS_G2D_SCREENCOLORMODE_16x16)),
                      "Color mode mismatch between character data and screen data");
    NNS_G2D_ASSERTMSG(pPltData == NULL
                      || ((pPltData->fmt == GX_TEXFMT_PLTT16)
                          == (pScnData->colorMode == NNS_G2D_SCREENCOLORMODE_16x16)),
                      "Color mode mismatch between palette data and screen data");
    NNS_G2D_ASSERT(pChrData == NULL
                   || (NNSi_G2dGetCharacterFmtType(pChrData->characterFmt)
                       == NNS_G2D_CHARACTER_FMT_CHAR));

    SetBGControlAuto(
        bg,
        NNSi_G2dBGGetScreenFormat(pScnData),
        NNSi_G2dBGGetScreenColorMode(pScnData),
        pScnData->screenWidth,
        pScnData->screenHeight,
        scnBase,
        chrBase);
    NNS_G2dBGLoadElementsEx(bg, pScnData, pChrData, pPltData, pPosInfo, pCmpInfo);
}

void NNS_G2dBGLoadScreenRect (void * pScreenDst, const NNSG2dScreenData * pScnData, int srcX, int srcY, int dstX, int dstY, int dstW, int dstH, int width, int height)
{
    NNS_G2D_POINTER_ASSERT(pScreenDst);
    NNS_G2D_POINTER_ASSERT(pScnData);

    if (dstX < 0) {
        const int adj = -dstX;
        srcX += adj;
        width -= adj;
        dstX = 0;
    }
    if (dstY < 0) {
        const int adj = -dstY;
        srcY += adj;
        height -= adj;
        dstY = 0;
    }
    if (dstX + width > dstW) {
        const int adj = (dstX + width) - dstW;
        width -= adj;
    }
    if (dstY + height > dstH) {
        const int adj = (dstY + height) - dstH;
        height -= adj;
    }
    if (srcX < 0) {
        const int adj = -srcX;
        dstX += adj;
        width -= adj;
        srcX = 0;
    }
    if (srcY < 0) {
        const int adj = -srcY;
        dstY += adj;
        height -= adj;
        srcY = 0;
    }
    if (srcX + width > pScnData->screenWidth / 8) {
        const int adj = (srcX + width) - (pScnData->screenWidth / 8);
        width -= adj;
    }
    if (srcY + height > pScnData->screenHeight / 8) {
        const int adj = (srcY + height) - (pScnData->screenHeight / 8);
        height -= adj;
    }

    if (width <= 0 || height <= 0) {
        return;
    }

    switch (pScnData->screenFormat) {
    case NNS_G2D_SCREENFORMAT_TEXT:
        LoadScreenPartText(pScreenDst, pScnData, srcX, srcY, dstX, dstY, dstW, dstH, width, height);
        break;
    case NNS_G2D_SCREENFORMAT_AFFINE:
        LoadScreenPartAffine(pScreenDst, pScnData, srcX, srcY, dstX, dstY, dstW, width, height);
        break;
    case NNS_G2D_SCREENFORMAT_AFFINEEXT:
        LoadScreenPart256x16Pltt(pScreenDst, pScnData, srcX, srcY, dstX, dstY, dstW, width, height);
        break;
    default:
        NNS_G2D_ASSERTMSG(FALSE, "Unknown screen format(=%d)", pScnData->screenFormat);
    }
}
