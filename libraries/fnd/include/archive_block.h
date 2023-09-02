#ifndef NNS_FND_ARCHIVE_BLOCK_H_
#define NNS_FND_ARCHIVE_BLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 blockType;
    u32 blockSize;
} NNSiFndArchiveBlockHeader;

typedef NNSiFndArchiveBlockHeader	NNSiFndArchiveDirBlockHeader;
typedef NNSiFndArchiveBlockHeader	NNSiFndArchiveImgBlockHeader;

typedef struct {
    u32	blockType;
    u32	blockSize;
    u16	numFiles;
    u16	reserved;
} NNSiFndArchiveFatBlockHeader;

#ifdef __cplusplus
}
#endif

#endif
