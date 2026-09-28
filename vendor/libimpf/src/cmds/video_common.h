#ifndef __VIDEO_COMMON_H__
#define __VIDEO_COMMON_H__


#include <impf/media_manager.h>


#define BITRATE_720P_Kbs                1000



void MakeTables(int q, uint8_t *lqt, uint8_t *cqt);

void encodeConfig(IMPF_FuncPipe_EncChn *chn, IMPPayloadType enType, IMPEncoderRcMode rcMode);


#endif
