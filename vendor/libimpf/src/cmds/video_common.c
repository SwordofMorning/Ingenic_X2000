/*
*  Copyright (C) 2018, <kai.shen@ingenic.com>
*
*  Ingenic IMP samples project
*
*  This program is free software; you can redistribute it and/or modify it
*  under  the terms of the GNU General  Public License as published by the
*  Free Software Foundation;  either version 2 of the License, or (at your
*  option) any later version.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  675 Mass Ave, Cambridge, MA 02139, USA.
*
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "video_common.h"
#include "sensor_info.h"



static const int jpeg_chroma_quantizer[64] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

static const int jpeg_luma_quantizer[64] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/*改变q的值可得到不同质量的jpeg,q越大jpeg质量越好，q的范围1~99*/
void MakeTables(int q, uint8_t *lqt, uint8_t *cqt)
{
    int i;
    int factor = q;
    if (q < 1) factor = 1;
    if (q > 99) factor = 99;
    if (q < 50)
        q = 5000 / factor;
    else
        q = 200 - factor*2;
    for (i=0; i < 64; i++) {
        int lq = (jpeg_luma_quantizer[i] * q + 50) / 100;
        int cq = (jpeg_chroma_quantizer[i] * q + 50) / 100;
        /* Limit the quantizers to 1 <= q <= 255 */
        if (lq < 1) lq = 1;
        else if (lq > 255) lq = 255;
        lqt[i] = lq;
        if (cq < 1) cq = 1;
        else if (cq > 255) cq = 255;
        cqt[i] = cq;
    }
}


void encodeConfig(IMPF_FuncPipe_EncChn *chn, IMPPayloadType enType, IMPEncoderRcMode rcMode)
{
    IMPEncoderAttr *encAttr = &chn->encChnAttr.encAttr;
    IMPEncoderRcAttr *rcAttr = &chn->encChnAttr.rcAttr;

    memset(&chn->encChnAttr, 0, sizeof(IMPEncoderCHNAttr));
    encAttr->enType = enType;
    encAttr->bufSize = 0;
    encAttr->profile = 2;
    encAttr->picWidth = SENSOR_WIDTH;
    encAttr->picHeight = SENSOR_HEIGHT;

    //码率控制
    if (encAttr->enType == PT_JPEG) {
        ;
    } else if (encAttr->enType == PT_H264) {
        rcAttr->outFrmRate.frmRateNum = SENSOR_FPS_NUM;
        rcAttr->outFrmRate.frmRateDen = SENSOR_FPS_DEN;
        rcAttr->maxGop = 2 * rcAttr->outFrmRate.frmRateNum / rcAttr->outFrmRate.frmRateDen;
        if (rcMode == ENC_RC_MODE_CBR) {
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
            rcAttr->attrRcMode.attrH264Cbr.outBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH264Cbr.maxQp = 45;
            rcAttr->attrRcMode.attrH264Cbr.minQp = 15;
            rcAttr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH264Cbr.frmQPStep = 3;
            rcAttr->attrRcMode.attrH264Cbr.gopQPStep = 15;
            rcAttr->attrRcMode.attrH264Cbr.adaptiveMode = false;
            rcAttr->attrRcMode.attrH264Cbr.gopRelation = false;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else if (rcMode == ENC_RC_MODE_VBR) {
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
            rcAttr->attrRcMode.attrH264Vbr.maxQp = 45;
            rcAttr->attrRcMode.attrH264Vbr.minQp = 15;
            rcAttr->attrRcMode.attrH264Vbr.staticTime = 2;
            rcAttr->attrRcMode.attrH264Vbr.maxBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH264Vbr.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH264Vbr.changePos = 80;
            rcAttr->attrRcMode.attrH264Vbr.qualityLvl = 2;
            rcAttr->attrRcMode.attrH264Vbr.frmQPStep = 3;
            rcAttr->attrRcMode.attrH264Vbr.gopQPStep = 15;
            rcAttr->attrRcMode.attrH264Vbr.gopRelation = false;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else if (rcMode == ENC_RC_MODE_SMART) {
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_SMART;
            rcAttr->attrRcMode.attrH264Smart.maxQp = 45;
            rcAttr->attrRcMode.attrH264Smart.minQp = 15;
            rcAttr->attrRcMode.attrH264Smart.staticTime = 2;
            rcAttr->attrRcMode.attrH264Smart.maxBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH264Smart.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH264Smart.changePos = 80;
            rcAttr->attrRcMode.attrH264Smart.qualityLvl = 2;
            rcAttr->attrRcMode.attrH264Smart.frmQPStep = 3;
            rcAttr->attrRcMode.attrH264Smart.gopQPStep = 15;
            rcAttr->attrRcMode.attrH264Smart.gopRelation = false;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = rcAttr->maxGop - 1;
            rcAttr->attrHSkip.hSkipAttr.n = 1;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 6;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else { /* fixQp */
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_FIXQP;
            rcAttr->attrRcMode.attrH264FixQp.qp = 35;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        }
    }
#if 0
    else { //PT_H265
        rcAttr->outFrmRate.frmRateNum = SENSOR_FPS_NUM;
        rcAttr->outFrmRate.frmRateDen = SENSOR_FPS_DEN;
        rcAttr->maxGop = 2 * rcAttr->outFrmRate.frmRateNum / rcAttr->outFrmRate.frmRateDen;
        if (rcMode == ENC_RC_MODE_CBR) {
            rcAttr->attrRcMode.attrH265Cbr.maxQp = 45;
            rcAttr->attrRcMode.attrH265Cbr.minQp = 15;
            rcAttr->attrRcMode.attrH265Cbr.staticTime = 2;
            rcAttr->attrRcMode.attrH265Cbr.outBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH265Cbr.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH265Cbr.frmQPStep = 3;
            rcAttr->attrRcMode.attrH265Cbr.gopQPStep = 15;
            rcAttr->attrRcMode.attrH265Cbr.flucLvl = 2;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else if (rcMode == ENC_RC_MODE_VBR) {
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
            rcAttr->attrRcMode.attrH265Vbr.maxQp = 45;
            rcAttr->attrRcMode.attrH265Vbr.minQp = 15;
            rcAttr->attrRcMode.attrH265Vbr.staticTime = 2;
            rcAttr->attrRcMode.attrH265Vbr.maxBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH265Vbr.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH265Vbr.changePos = 80;
            rcAttr->attrRcMode.attrH265Vbr.qualityLvl = 2;
            rcAttr->attrRcMode.attrH265Vbr.frmQPStep = 3;
            rcAttr->attrRcMode.attrH265Vbr.gopQPStep = 15;
            rcAttr->attrRcMode.attrH265Vbr.flucLvl = 2;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else if (rcMode == ENC_RC_MODE_SMART) {
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_SMART;
            rcAttr->attrRcMode.attrH265Smart.maxQp = 45;
            rcAttr->attrRcMode.attrH265Smart.minQp = 15;
            rcAttr->attrRcMode.attrH265Smart.staticTime = 2;
            rcAttr->attrRcMode.attrH265Smart.maxBitRate = BITRATE_720P_Kbs * (encAttr->picWidth * encAttr->picHeight) / (1280 * 720);
            rcAttr->attrRcMode.attrH265Smart.iBiasLvl = 0;
            rcAttr->attrRcMode.attrH265Smart.changePos = 80;
            rcAttr->attrRcMode.attrH265Smart.qualityLvl = 2;
            rcAttr->attrRcMode.attrH265Smart.frmQPStep = 3;
            rcAttr->attrRcMode.attrH265Smart.gopQPStep = 15;
            rcAttr->attrRcMode.attrH265Smart.flucLvl = 2;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = rcAttr->maxGop - 1;
            rcAttr->attrHSkip.hSkipAttr.n = 1;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 6;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        } else { /* fixQp */
            rcAttr->attrRcMode.rcMode = ENC_RC_MODE_FIXQP;
            rcAttr->attrRcMode.attrH265FixQp.qp = 35;

            rcAttr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
            rcAttr->attrHSkip.hSkipAttr.m = 0;
            rcAttr->attrHSkip.hSkipAttr.n = 0;
            rcAttr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
            rcAttr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
            rcAttr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
            rcAttr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
        }
    }
#endif
}



