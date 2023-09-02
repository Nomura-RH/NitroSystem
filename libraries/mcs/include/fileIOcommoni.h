#ifndef NNS_MCS_FILEIOCOMMONI_H_
#define NNS_MCS_FILEIOCOMMONI_H_

#if defined(_MSC_VER)
    #include "mcsStdInt.h"
#else
    #include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NNSi_MCS_FILEIO_CHANNEL  (uint16_t)('FI' + 0x8000)

enum {
    NNSi_MCS_FILEIO_CMD_FILEOPEN,
    NNSi_MCS_FILEIO_CMD_FILECLOSE,
    NNSi_MCS_FILEIO_CMD_FILEREAD,
    NNSi_MCS_FILEIO_CMD_FILEWRITE,
    NNSi_MCS_FILEIO_CMD_BROWSEFILE,
    NNSi_MCS_FILEIO_CMD_FINDFIRST,
    NNSi_MCS_FILEIO_CMD_FINDNEXT,
    NNSi_MCS_FILEIO_CMD_FINDCLOSE,
    NNSi_MCS_FILEIO_CMD_FILESEEK
};

enum {
    NNSi_MCS_FILEIO_OPEN_DIRECT,
    NNSi_MCS_FILEIO_OPEN_DIALOG
};

#ifdef _WIN32
    typedef uint32_t NNSiMcsVoidPtr;
    typedef uint32_t NNSiMcsFilePtr;
    typedef uint32_t NNSiMcsFileFindDataPtr;
#else
    typedef void * NNSiMcsVoidPtr;
    typedef NNSMcsFile * NNSiMcsFilePtr;
    typedef NNSMcsFileFindData * NNSiMcsFileFindDataPtr;
#endif

typedef struct NNSiMcsFileIOCmdHeader NNSiMcsFileIOCmdHeader;
struct NNSiMcsFileIOCmdHeader {
    uint16_t command;
    uint16_t type;
};

typedef struct NNSiMcsFileIOCmdOpen NNSiMcsFileIOCmdOpen;
struct NNSiMcsFileIOCmdOpen {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    uint32_t flag;
    char filename[NNS_MCS_FILEIO_PATH_MAX];
};

typedef struct NNSiMcsFileIOCmdClose NNSiMcsFileIOCmdClose;
struct NNSiMcsFileIOCmdClose {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    uint32_t handle;
};

typedef struct NNSiMcsFileIOCmdRead NNSiMcsFileIOCmdRead;
struct NNSiMcsFileIOCmdRead {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    uint32_t handle;
    NNSiMcsVoidPtr pBuffer;
    uint32_t size;
};

typedef struct NNSiMcsFileIOCmdWrite NNSiMcsFileIOCmdWrite;
struct NNSiMcsFileIOCmdWrite {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    uint32_t handle;
    uint32_t size;
};

typedef struct NNSiMcsFileIOCmdFileSeek NNSiMcsFileIOCmdFileSeek;
struct NNSiMcsFileIOCmdFileSeek {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    uint32_t handle;
    int32_t distanceToMove;
    uint32_t moveMethod;
};

typedef struct NNSiMcsFileIOCmdFindFirst NNSiMcsFileIOCmdFindFirst;
struct NNSiMcsFileIOCmdFindFirst {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    NNSiMcsFileFindDataPtr pFindData;
    char pPattern[NNS_MCS_FILEIO_PATH_MAX];
};

typedef struct NNSiMcsFileIOCmdFindNext NNSiMcsFileIOCmdFindNext;
struct NNSiMcsFileIOCmdFindNext {
    uint16_t command;
    uint16_t type;
    NNSiMcsFilePtr pFile;
    NNSiMcsFileFindDataPtr pFindData;
    uint32_t handle;
};

typedef struct NNSiMcsFileIOResult NNSiMcsFileIOResult;
struct NNSiMcsFileIOResult {
    uint16_t command;
    uint16_t result;
    uint32_t srvErrCode;
    NNSiMcsFilePtr pFile;
};

typedef struct NNSiMcsFileIOResultOpen NNSiMcsFileIOResultOpen;
struct NNSiMcsFileIOResultOpen {
    uint16_t command;
    uint16_t result;
    uint32_t srvErrCode;
    NNSiMcsFilePtr pFile;
    uint32_t handle;
    uint32_t filesize;
};

typedef struct NNSiMcsFileIOResultRead NNSiMcsFileIOResultRead;
struct NNSiMcsFileIOResultRead {
    uint16_t command;
    uint16_t result;
    uint32_t srvErrCode;
    NNSiMcsFilePtr pFile;
    NNSiMcsVoidPtr pBuffer;
    uint32_t size;
    uint32_t totalSize;
};

typedef struct NNSiMcsFileIOResultFileSeek NNSiMcsFileIOResultFileSeek;
struct NNSiMcsFileIOResultFileSeek {
    uint16_t command;
    uint16_t result;
    uint32_t srvErrCode;
    NNSiMcsFilePtr pFile;
    uint32_t filePointer;
};

typedef struct NNSiMcsFileIOResultFind NNSiMcsFileIOResultFind;
struct NNSiMcsFileIOResultFind {
    uint16_t command;
    uint16_t result;
    uint32_t srvErrCode;
    NNSiMcsFilePtr pFile;
    NNSiMcsFileFindDataPtr pFindData;
    uint32_t handle;
    uint32_t filesize;
    uint32_t attribute;
    char pFilename[NNS_MCS_FILEIO_PATH_MAX];
};

#ifdef __cplusplus
}
#endif

#endif
