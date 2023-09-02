#ifndef NNS_MCS_BASECOMMONI_H_
#define NNS_MCS_BASECOMMONI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NNSi_MCS_ENS_READ_RB_SIZE       (64 * 1024)
#define NNSi_MCS_ENS_READ_RB_START      0x10000000
#define NNSi_MCS_ENS_READ_RB_END        (NNSi_MCS_ENS_READ_RB_START + NNSi_MCS_ENS_READ_RB_SIZE)

#define NNSi_MCS_ENS_WRITE_RB_SIZE      NNSi_MCS_ENS_READ_RB_SIZE
#define NNSi_MCS_ENS_WRITE_RB_START     NNSi_MCS_ENS_READ_RB_END
#define NNSi_MCS_ENS_WRITE_RB_END       (NNSi_MCS_ENS_WRITE_RB_START + NNSi_MCS_ENS_WRITE_RB_SIZE)

#define NNSi_MCS_MSG_HEAD_VERSION       1

static const uint16_t NNSi_MCS_SYSMSG_CHANNEL = 0xFFFF;

typedef struct NNSiMcsEnsMsgBuf NNSiMcsEnsMsgBuf;
struct NNSiMcsEnsMsgBuf {
    uint32_t channel;
    uint8_t buf[NNSi_MCS_MESSAGE_SIZE_MAX];
};

#ifdef __cplusplus
}
#endif

#endif
