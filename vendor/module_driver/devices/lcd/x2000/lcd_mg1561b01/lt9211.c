#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <utils/gpio.h>
#include <utils/i2c.h>

#include "lt9211.h"
static unsigned char mipi_fmt = 0;

struct video_timing *pVideo_Format;
#define HTOTAL (HFP + HACT + HBP + HS)
#define VTOTAL (VFP + VACT + VBP + VS)

#define PIXCLK (HTOTAL * VTOTAL * FPS / 1000)

#if 0
#define FPS    30
#define HACT   1920
#define VACT   1080

#define HFP    88
#define HBP    148
#define HS     44

#define VFP    4
#define VBP    36
#define VS     5
#endif

//hfp, hs, hbp, hact, htotal, vfp, vs, vbp, vact, vtotal, pixclk
struct video_timing video_640x480_60Hz     ={ 8, 96,  40, 640,   800, 33,  2,  10, 480,   525,  25000};
struct video_timing video_720x480_60Hz     ={16, 62,  60, 720,   858,  9,  6,  30, 480,   525,  27000};
struct video_timing video_1280x720_60Hz    ={110,40, 220,1280,  1650,  5,  5,  20, 720,   750,  74250};
struct video_timing video_1280x720_30Hz    ={110,40, 220,1280,  1650,  5,  5,  20, 720,   750,  37125};
struct video_timing video_1366x768_60Hz    ={26, 110,110,1366,  1592,  13, 6,  13, 768,   800,  81000};
struct video_timing video_1920x720_60Hz    ={148,44, 88, 1920,  2200, 28,  5,  12, 720,   765,  88000};
struct video_timing video_1920x1080_30Hz   ={88, 44, 148,1920,  2200,  4,  5,  36, 1080, 1125,  74250};
struct video_timing video_1920x1080_60Hz   ={HFP, HS, HBP, HACT, HTOTAL, VFP, VS, VBP, VACT, VTOTAL, PIXCLK};
//struct video_timing video_1920x1080_60Hz   ={90, 100, 90, 1920, 2200, 20, 5, 20, 1080, 1125, 148500};//60fps
//struct video_timing video_1920x1080_60Hz   ={90, 100, 90, 1920, 2200, 20, 5, 20, 1080, 1125, 74250};//30fps
struct video_timing video_1920x1200_60Hz   ={48, 32,  80,1920,  2080,  3,  6,  26, 1200, 1235, 154000};
struct video_timing video_3840x2160_30Hz   ={176,88, 296,3840,  4400,  8,  10, 72, 2160, 2250, 297000};

static int lt9211_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = client->addr,
        .buf    = buf,
        .len    = 2,
        .flags  = 0,
    };

    int ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline char lt9211_i2c_read(struct i2c_client *i2c, unsigned char addr)
{
    char value = 0xff;

    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &addr,
        },
        [1] = {
            .addr   = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = &value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "lt9211: failed to read reg: %x\n", (int )addr);

    return value;
}

#ifdef DEBUG
static void LT9211_ChipID(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x81);//register bank
    printk("LT9211 Chip ID:%x,",lt9211_i2c_read(client, 0x00));
    printk("%02x, ",lt9211_i2c_read(client, 0x01));
    printk("%02x\n",lt9211_i2c_read(client, 0x02));
}
#endif

/** video chk soft rst **/
#if 0
static void lt9211_vid_chk_rst(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x10,0xbe);
    usleep_range(10*1000, 10*1000);
    lt9211_i2c_write(client, 0x10,0xfe);
}
#endif

/** lvds rx logic rst **/
static void lt9211_mipirx_logic_rst(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x0a,0xc0);
    lt9211_i2c_write(client, 0x20,0xbf);  //mipi rx div logic reset,for portb input
    usleep_range(10*1000, 10*1000);
    lt9211_i2c_write(client, 0x0a,0xc1);
    lt9211_i2c_write(client, 0x20,0xff);
}

static void LT9211_SystemInt(struct i2c_client *client)
{
    /* system clock init */
    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x01,0x18);

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x06,0x61);
    lt9211_i2c_write(client, 0x07,0xa8); //fm for sys_clk

    lt9211_i2c_write(client, 0xff,0x87);
    lt9211_i2c_write(client, 0x14,0x08); //default value
    lt9211_i2c_write(client, 0x15,0x00); //default value
    lt9211_i2c_write(client, 0x18,0x0f);
    lt9211_i2c_write(client, 0x22,0x08); //default value
    lt9211_i2c_write(client, 0x23,0x00); //default value
    lt9211_i2c_write(client, 0x26,0x0f);

    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x0B,0xFE); //rpt reset
}

#if 0
static void LT9211_ClkDetDebug(struct i2c_client *client)
{
    unsigned int fm_value;

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x00,0x14);
    msleep(100);
    fm_value = 0;
    fm_value = (lt9211_i2c_read(client, 0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x0a);

    printk("input ttlclk: \n");
    printk("\033[33m fm_value = %x |\033[0m\n", fm_value);
}
#endif

static void LT9211_MipiRxPhy(struct i2c_client *client)
{
#ifdef INPUT_PORTA
    debug("Port A PHY Config\n");
    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x02,0x44);  //Port A MIPI mode enable
    lt9211_i2c_write(client, 0x04,0xa0);  //select port A clk as byteclk
    lt9211_i2c_write(client, 0x05,0x22);  //port A CLK lane swap
    lt9211_i2c_write(client, 0x07,0x9f);  //port A clk enable
    lt9211_i2c_write(client, 0x08,0xfc);  //port A clk enable
    lt9211_i2c_write(client, 0x09,0x01);  //port A P/N swap
    lt9211_i2c_write(client, 0x17,0x0c);

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x33,0x1b); //port a lane swap    1b:no swap
#endif

#ifdef INPUT_PORTB
    debug("Port B PHY Config\n");
    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x02,0x44);   //Port A/B MIPI mode enable
    lt9211_i2c_write(client, 0x04,0xa1);  //select port A clk as byteclk
    lt9211_i2c_write(client, 0x05,0x26);  //port A CLK lane swap
    lt9211_i2c_write(client, 0x0d,0x26);   //port B CLK lane swap
    lt9211_i2c_write(client, 0x07,0x9f);   //port A clk enable  (??Portb?,porta?lane0 clk???)
    lt9211_i2c_write(client, 0x0f,0x9f);   //port B clk enable
    lt9211_i2c_write(client, 0x10,0xfc);   //select port B clk as byteclk
    lt9211_i2c_write(client, 0x11,0x01);   //port B P/N swap
    lt9211_i2c_write(client, 0x17,0x0c);
    lt9211_i2c_write(client, 0x1d,0x0c);

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x34,0x1b);   //Port B Lane swap
#endif

    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x20,0x7f);
    lt9211_i2c_write(client, 0x20,0xff);  //mlrx calib reset
}

static void LT9211_MipiRxDigital(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x86);
#ifdef INPUT_PORTA
    lt9211_i2c_write(client, 0x30,0x85);      //mipirx input port sel
#endif

#ifdef INPUT_PORTB
    lt9211_i2c_write(client, 0x30,0x8f);      //mipirx input port sel
#endif

    lt9211_i2c_write(client, 0xff,0x85);
    lt9211_i2c_write(client, 0x88,0x40);       //select mipi in lvds out

#ifdef MIPI_CSI
    debug("Set to CSI Mode\n");
    lt9211_i2c_write(client, 0xff,0xd0);  //CSI_EN
    lt9211_i2c_write(client, 0x04,0x10);
    lt9211_i2c_write(client, 0x21,0xc6);  //CSI_SEL
#else
    debug("Set to DSI Mode\n");
#endif

    lt9211_i2c_write(client, 0xff,0xd0);
    lt9211_i2c_write(client, 0x00,0x02);        //4Lane:0x00, 2Lane:0x02, 1Lane:0x01
    lt9211_i2c_write(client, 0x02,0x05);        //settle
}

static void LT9211_SetVideoTiming(struct i2c_client *client, struct video_timing *video_format)
{
    msleep(100);
    lt9211_i2c_write(client, 0xff,0xd0);
    lt9211_i2c_write(client, 0x0d,(unsigned char)(video_format->vtotal>>8)); //vtotal[15:8]
    lt9211_i2c_write(client, 0x0e,(unsigned char)(video_format->vtotal)); //vtotal[7:0]
    lt9211_i2c_write(client, 0x0f,(unsigned char)(video_format->vact>>8)); //vactive[15:8]
    lt9211_i2c_write(client, 0x10,(unsigned char)(video_format->vact)); //vactive[7:0]
    lt9211_i2c_write(client, 0x15,(unsigned char)(video_format->vs)); //vs[7:0]
    lt9211_i2c_write(client, 0x17,(unsigned char)(video_format->vfp>>8)); //vfp[15:8]
    lt9211_i2c_write(client, 0x18,(unsigned char)(video_format->vfp)); //vfp[7:0]

    lt9211_i2c_write(client, 0x11,(unsigned char)(video_format->htotal>>8)); //htotal[15:8]
    lt9211_i2c_write(client, 0x12,(unsigned char)(video_format->htotal)); //htotal[7:0]
    lt9211_i2c_write(client, 0x13,(unsigned char)(video_format->hact>>8)); //hactive[15:8]
    lt9211_i2c_write(client, 0x14,(unsigned char)(video_format->hact)); //hactive[7:0]
    lt9211_i2c_write(client, 0x16,(unsigned char)(video_format->hs)); //hs[7:0]
    lt9211_i2c_write(client, 0x19,(unsigned char)(video_format->hfp>>8)); //hfp[15:8]
    lt9211_i2c_write(client, 0x1a,(unsigned char)(video_format->hfp)); //hfp[7:0]
}

#ifdef DEBUG
static void LT9211_debug(struct i2c_client *client)
{
    int i;
    unsigned char start = 0x80;
    lt9211_i2c_write(client, 0xff,0xd0);
    for(i=0;i<30;i++)
        printk("read 0x%02x = 0x%02x\n", start+i, lt9211_i2c_read(client, start+i));
}
#endif

static int LT9211_TimingSet(struct i2c_client *client)
{
    unsigned short hact ;
    unsigned short vact ;
    unsigned char pa_lpn = 0;

    lt9211_mipirx_logic_rst(client);
    msleep(100);

    lt9211_i2c_write(client, 0xff,0xd0);
    hact = ((lt9211_i2c_read(client, 0x82) & 0xff)<<8) + (lt9211_i2c_read(client, 0x83) & 0xff);
    mipi_fmt = (lt9211_i2c_read(client, 0x84) & 0x0f);
    vact = ((lt9211_i2c_read(client, 0x85) & 0xff)<<8) +(lt9211_i2c_read(client, 0x86) & 0xff);
    pa_lpn = lt9211_i2c_read(client, 0x9c);
#ifdef DEBUG
    LT9211_debug(client);
#endif

    debug("\033[33m hact = %d |\033[0m\n", hact);

    if(mipi_fmt == 0x03) {
        debug("Input MIPI FMT: CSI_YUV422_16\n");
        hact = hact / 2;
    } else if(mipi_fmt == 0x0a) {
        debug("Input MIPI FMT: RGB888\n");
        hact = hact / 3;
    }

    debug("\033[33m hact = %d |\033[0m\n", hact);
    debug("\033[33m vact = %d |\033[0m\n", vact);

    debug("fmt = %x\n", mipi_fmt);
    debug("pa_lpn = %x\n", pa_lpn);

    msleep(100);
    if ((hact == video_1280x720_60Hz.hact ) &&( vact == video_1280x720_60Hz.vact )) {
        pVideo_Format = &video_1280x720_60Hz;
        LT9211_SetVideoTiming(client, &video_1280x720_60Hz);
    } else if ((hact == video_1366x768_60Hz.hact ) &&( vact == video_1366x768_60Hz.vact )) {
        pVideo_Format = &video_1366x768_60Hz;
        LT9211_SetVideoTiming(client, &video_1366x768_60Hz);
    } else if ((hact == video_1920x1080_60Hz.hact ) &&( vact == video_1920x1080_60Hz.vact )) {
        pVideo_Format = &video_1920x1080_60Hz;
        LT9211_SetVideoTiming(client, &video_1920x1080_60Hz);
    } else if ((hact == video_1920x1200_60Hz.hact ) &&( vact == video_1920x1200_60Hz.vact )) {
        pVideo_Format = &video_1920x1200_60Hz;
        LT9211_SetVideoTiming(client, &video_1920x1200_60Hz);
    } else {
        pVideo_Format = NULL;
        printk("video_none: do not support hact(%d) and vact(%d)\n", hact, vact);
        return -1;
    }

    return 0;
}

static void LT9211_DesscPll(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x2d,0x48);

    if(pVideo_Format->pclk_khz < 44000) {
        lt9211_i2c_write(client, 0x35,0x83);
    } else if(pVideo_Format->pclk_khz < 88000) {
        lt9211_i2c_write(client, 0x35,0x82);
    } else if(pVideo_Format->pclk_khz < 176000) {
        lt9211_i2c_write(client, 0x35,0x81);
    }
}

static int LT9211_MipiPcr(struct i2c_client *client)
{
    unsigned char loopx;
    lt9211_i2c_write(client, 0xff,0xd0);
    lt9211_i2c_write(client, 0x26,0x17);
    lt9211_i2c_write(client, 0x27,0xC3);
    lt9211_i2c_write(client, 0x2d,0x30);  //PCR M overflow limit setting.
    lt9211_i2c_write(client, 0x31,0x10);  //PCR M underflow limit setting.
    lt9211_i2c_write(client, 0x23,0x20);

    ///////////////////////////If the image shakes left and right, modify here//////////////////////////
    debug("0x0c: 0x%02x, 0x0c: 0x%02x\n", lt9211_i2c_read(client, 0x0c), lt9211_i2c_read(client, 0x1c));
    lt9211_i2c_write(client, 0x0c,0x30);
    lt9211_i2c_write(client, 0x1c,0x30);
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    lt9211_i2c_write(client, 0x38,0x02);
    lt9211_i2c_write(client, 0x39,0x10);
    lt9211_i2c_write(client, 0x3a,0x20);
    lt9211_i2c_write(client, 0x3b,0x60);
    lt9211_i2c_write(client, 0x3f,0x04);
    lt9211_i2c_write(client, 0x40,0x08);
    lt9211_i2c_write(client, 0x41,0x10);

    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x0B,0xEE);
    lt9211_i2c_write(client, 0x0B,0xFE);

    for(loopx = 0; loopx < 5; loopx++) //Check pcr_stable
    {
        msleep(200);
        lt9211_i2c_write(client, 0xff,0xd0);
        if(lt9211_i2c_read(client, 0x87)&0x08) {
            debug("\033[32mLT9211 pcr stable\033[0m\n");
            return 0;
        }
    }
    printk("LT9211 pcr unstable!!!!\n");
    return -1;
}

static void LT9211_CSC(struct i2c_client *client)
{
    //yuv422 to rgb888
    if(mipi_fmt == 0x03) {
        debug("csc:yuv422 to rgb888\n");
        lt9211_i2c_write(client, 0xff,0xf9);
        lt9211_i2c_write(client, 0x90,0x03);
        lt9211_i2c_write(client, 0x91,0x03);
    }
}

static void LT9211_TxPhy(struct i2c_client *client)
{
    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x62,0x00); //ttl output disable
    if( LVDS_PORTNUM == LVDS_2PORT ) {
        lt9211_i2c_write(client, 0x3b,0xb8);
    } else {
        lt9211_i2c_write(client, 0x3b,0x38);  //dual-port lvds tx phy
    }
    lt9211_i2c_write(client, 0x3e,0x92);
    lt9211_i2c_write(client, 0x3f,0x48);
    lt9211_i2c_write(client, 0x40,0x31);
    lt9211_i2c_write(client, 0x43,0x80);
    lt9211_i2c_write(client, 0x44,0x00);
    lt9211_i2c_write(client, 0x45,0x00);
    lt9211_i2c_write(client, 0x49,0x00);
    lt9211_i2c_write(client, 0x4a,0x01);
    lt9211_i2c_write(client, 0x4e,0x00);
    lt9211_i2c_write(client, 0x4f,0x00);
    lt9211_i2c_write(client, 0x50,0x00);
    lt9211_i2c_write(client, 0x53,0x00);
    lt9211_i2c_write(client, 0x54,0x01);

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x46,0x10);
#ifdef LVDS_2PORT_SWAP
    debug("LVDS Output Port Swap!\n");
    lt9211_i2c_write(client, 0x46,0x40);
#endif

    lt9211_i2c_write(client, 0xff,0x81);
    lt9211_i2c_write(client, 0x20,0x7b);
    lt9211_i2c_write(client, 0x20,0xff); //mlrx mltx calib reset
}

static void LT9211_TxDigital(struct i2c_client *client)
{
    debug("LT9211 LVDS_OUTPUT_MODE: \n");
    lt9211_i2c_write(client, 0xff,0x85); /* lvds tx controller */
    lt9211_i2c_write(client, 0x59,0x40);
    if( LVDS_DATAFORMAT == VESA ) {
        debug("Data Format: VESA\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) & 0x7f));
    } else if( LVDS_DATAFORMAT == JEIDA ) {
        debug("Data Format: JEIDA\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) | 0x80));
    }
    if( LVDS_COLORDEPTH == DEPTH_6BIT ) {
        debug("ColorDepth: 6Bit\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) & 0xef));
    } else if( LVDS_COLORDEPTH == DEPTH_8BIT ) {
        debug("ColorDepth: 8Bit\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) | 0x10));
    }
    if( LVDS_MODE == SYNC_MODE ) {
        debug("LVDS_MODE: Sync Mode\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) & 0xdf));
    } else if( LVDS_MODE == DE_MODE ) {
        debug("LVDS_MODE: De Mode\n");
        lt9211_i2c_write(client, 0x59, (lt9211_i2c_read(client, 0x59) | 0x20));
    }

    lt9211_i2c_write(client, 0x5a,0xaa);
    lt9211_i2c_write(client, 0x5b,0xaa);
    if( LVDS_PORTNUM == LVDS_2PORT ) {
        debug("LVDS Output Port Num: 2Port\n");
        lt9211_i2c_write(client, 0x5c,0x01);    //lvdstx port sel 01:dual;00:single
    } else {
        debug("LVDS Output Port Num: 1Port\n");
        lt9211_i2c_write(client, 0x5c,0x00);
    }
    lt9211_i2c_write(client, 0xa1,0x77);
    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x40,0x40); //tx_src_sel
    /*port src sel*/
    lt9211_i2c_write(client, 0x41,0x34);
    lt9211_i2c_write(client, 0x42,0x10);
    lt9211_i2c_write(client, 0x43,0x23); //pt0_tx_src_sel
    lt9211_i2c_write(client, 0x44,0x41);
    lt9211_i2c_write(client, 0x45,0x02); //pt1_tx_src_scl
}

static void LT9211_Txpll(struct i2c_client *client)
{
    unsigned char loopx;

    lt9211_i2c_write(client, 0xff,0x82);
    lt9211_i2c_write(client, 0x36,0x01); //b7:txpll_pd
    if( LVDS_PORTNUM == LVDS_1PORT ) {
        lt9211_i2c_write(client, 0x37,0x29);
    } else {
        lt9211_i2c_write(client, 0x37,0x2a);
    }
    lt9211_i2c_write(client, 0x38,0x06);
    lt9211_i2c_write(client, 0x39,0x30);
    lt9211_i2c_write(client, 0x3a,0x8e);
    lt9211_i2c_write(client, 0xff,0x87);
    lt9211_i2c_write(client, 0x37,0x14);
    lt9211_i2c_write(client, 0x13,0x00);
    lt9211_i2c_write(client, 0x13,0x80);
    msleep(100);
    for(loopx = 0; loopx < 10; loopx++) //Check Tx PLL cal
    {
        lt9211_i2c_write(client, 0xff,0x87);
        if(lt9211_i2c_read(client, 0x1f)& 0x80) {
            if(lt9211_i2c_read(client, 0x20)& 0x80) {
                debug("LT9211 tx pll lock\n");
            } else {
                printk("LT9211 tx pll unlocked\n");
            }
            debug("LT9211 tx pll cal done\n");
            break;
        } else {
            printk("LT9211 tx pll unlocked\n");
        }
    }
}

#ifdef DEBUG
static void LT9211_LvdsClkDebug(struct i2c_client *client)
{
    unsigned int fm_value;

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x00,0x12);
    msleep(100);
    fm_value = 0;
    fm_value = (lt9211_i2c_read(client, 0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x0a);

    printk("\033[33m lvds pixclk = %d |\033[0m\n", fm_value);
}

static void LT9211_MipiByteClkDebug(struct i2c_client *client)
{
    unsigned int fm_value;

    lt9211_i2c_write(client, 0xff,0x86);
#ifdef INPUT_PORTA
    lt9211_i2c_write(client, 0x00,0x01);
#endif
#ifdef INPUT_PORTB
    lt9211_i2c_write(client, 0x00,0x02);
#endif
    msleep(100);
    fm_value = 0;
    fm_value = (lt9211_i2c_read(client, 0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + lt9211_i2c_read(client, 0x0a);
    printk("\033[33m mipi byteclk = %d |\033[0m\n", fm_value);
}

static void LT9211_VideoCheckDebug(struct i2c_client *client)
{
    unsigned char sync_polarity;
    unsigned short hact, vact;
    unsigned short hs, vs;
    unsigned short hbp, vbp;
    unsigned short htotal, vtotal;
    unsigned short hfp, vfp;

    lt9211_i2c_write(client, 0xff,0x86);
    lt9211_i2c_write(client, 0x20,0x00);

    sync_polarity = lt9211_i2c_read(client, 0x70);
    vs = lt9211_i2c_read(client, 0x71);

    hs = lt9211_i2c_read(client, 0x72);
    hs = (hs<<8) + lt9211_i2c_read(client, 0x73);

    vbp = lt9211_i2c_read(client, 0x74);
    vfp = lt9211_i2c_read(client, 0x75);

    hbp = lt9211_i2c_read(client, 0x76);
    hbp = (hbp<<8) + lt9211_i2c_read(client, 0x77);

    hfp = lt9211_i2c_read(client, 0x78);
    hfp = (hfp<<8) + lt9211_i2c_read(client, 0x79);

    vtotal = lt9211_i2c_read(client, 0x7A);
    vtotal = (vtotal<<8) + lt9211_i2c_read(client, 0x7B);

    htotal = lt9211_i2c_read(client, 0x7C);
    htotal = (htotal<<8) + lt9211_i2c_read(client, 0x7D);

    vact = lt9211_i2c_read(client, 0x7E);
    vact = (vact<<8)+ lt9211_i2c_read(client, 0x7F);

    hact = lt9211_i2c_read(client, 0x80);
    hact = (hact<<8) + lt9211_i2c_read(client, 0x81);

    printk("sync_polarity = %x\n", sync_polarity);

    printk("\033[33m hfp = %d , hs = %d , hbp = %d , hact = %d , htotal = %d\033[0m\n", hfp, hs, hbp, hact, htotal);

    printk("\033[33m vfp = %d , vs = %d , vbp = %d , vact = %d , vtotal = %d\033[0m\n", vfp, vs, vbp, vact, vtotal);
}
#endif

int LT9211_MIPI2LVDS_Config(struct i2c_client *client)
{
    int retry = 5;
    int ret = 0;
    debug("*************LT9211 MIPI2LVDS Config*************\n");
#ifdef DEBUG
    LT9211_ChipID(client);
#endif
    LT9211_SystemInt(client);
    LT9211_MipiRxPhy(client);
    LT9211_MipiRxDigital(client);

    while(retry--)
        if ((ret = LT9211_TimingSet(client)) == 0) break;

    if( pVideo_Format != NULL )
    {
#ifdef DEBUG
        LT9211_MipiByteClkDebug(client);
#endif
        LT9211_DesscPll(client);

        retry = 5;
        while(retry--)
            if ((ret = LT9211_MipiPcr(client)) == 0) break;

        LT9211_CSC(client);
        /********LVDS OUTPUT CONFIG********/
        LT9211_TxPhy(client);
        LT9211_TxDigital(client);
        LT9211_Txpll(client);

#ifdef DEBUG
        /* lt9211_i2c_write(client, 0xff,0x85); */
        /* lt9211_i2c_write(client, 0x88,0xC0); // debug: colorbar */
        /* lt9211_i2c_write(client, 0xA1,0x04); // single color 0x1: blue, 0x2: green, 0x4: red. */

        LT9211_LvdsClkDebug(client);
        LT9211_VideoCheckDebug(client);
#endif
    }
    return ret;
}
