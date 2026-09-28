#ifndef __ETA6919_H__
#define __ETA6919_H__

static const unsigned int CS_IPRECHG[] = {
     20, 40, 60, 80,100,120,140,160,
    180,200,220,240,260
};

static const unsigned int CS_ITERM[] = {
     20, 40, 60, 80,100,120,140,160,
    180,200,220,240,260,280,300,320
};

static const unsigned int BAT_VREG[] = {
    3504,3600,3696,3800,3904,4000,4100,4150,
    4200,4300,4310,4320,4330,4340,4350,4360,
    4370,4380,4390,4400,4410,4420,4430,4440,
    4450,4460,4470,4480,4490,4500,4510,4520
};

static const unsigned int BAT_ICHG[] = {
       0,  20,  40,  60,  80, 100, 120, 140,
     160, 180, 200, 220, 240, 260, 280, 300,
     320, 340, 360, 380, 400, 420, 440, 460,
     480, 500, 520, 540, 560, 580, 600, 620,
     640, 660, 680, 700, 720, 740, 760, 780,
     800, 820, 840, 860, 880, 900, 920, 940,
     960, 980,1000,1020,1040,1060,1080,1100,
    1120,1140,1160,1180,1290,1360,1430,1500
};

static const unsigned int IINDPM_REG[] = {
     100, 200, 300, 400, 500, 600, 700, 800,
     900,1000,1100,1200,1300,1400,1500,1600,
    1700,1800,1900,2000,2100,2200,2300,2400,
    2500,2600,2700,2800,2900,3000,3100,3200,
};

static const unsigned int VINDPM_REG[] = {
    3900,4000,4100,4200,4300,4400,4500,4600,
    4700,4800,4900,5000,5100,5200,5300,5400
};

#define ETACON0     0x00
#define ETACON1     0x01
#define ETACON2     0x02
#define ETACON3     0x03
#define ETACON4     0x04
#define ETACON5     0x05
#define ETACON6     0x06
#define ETACON7     0x07
#define ETACON8     0x08
#define ETACON9     0x09
#define ETACONA     0x0A
#define ETACONB     0x0B
#define ETACONC     0x0C

/* ETACON0 */
#define ETACON0_EN_HIZ_MASK             0x01
#define ETACON0_EN_HIZ_SHIFT            7
#define ETACON0_TS_IGNORE_MASK          0x01
#define ETACON0_TS_IGNORE_SHIFT         6
#define ETACON0_BATSNS_DIS_MASK         0x01
#define ETACON0_BATSNS_DIS_SHIFT        5
#define ETACON0_IINDPM_MASK             0x1F
#define ETACON0_IINDPM_SHIFT            0

/* ETACON1 */
#define ETACON1_PFM_DIS_MASK            0x01
#define ETACON1_PFM_DIS_SHIFT           7
#define ETACON1_WDT_RST_MASK            0x01
#define ETACON1_WDT_RST_SHIFT           6
#define ETACON1_OTG_CONFIG_MASK         0x01
#define ETACON1_OTG_CONFIG_SHIFT        5
#define ETACON1_CHG_CONFIG_MASK         0x01
#define ETACON1_CHG_CONFIG_SHIFT        4
#define ETACON1_SYS_MIN_MASK            0x07
#define ETACON1_SYS_MIN_SHIFT           1
#define ETACON1_MIN_VBAT_SEL_MASK       0x01
#define ETACON1_MIN_VBAT_SEL_SHIFT      0

/* ETACON2 */
#define ETACON2_Q1_FULLON_MASK          0x01
#define ETACON2_Q1_FULLON_SHIFT         6
#define ETACON2_ICHG_MASK               0x3F
#define ETACON2_ICHG_SHIFT              0

/* ETACON3 */
#define ETACON3_IPRECHG_MASK            0x0F
#define ETACON3_IPRECHG_SHIFT           4
#define ETACON3_ITERM_MASK              0x0F
#define ETACON3_ITERM_SHIFT             0

/* ETACON4 */
#define ETACON4_VREG_MASK               0x1F
#define ETACON4_VREG_SHIFT              3
#define ETACON4_TOPOFF_TIMER_MASK       0x03
#define ETACON4_TOPOFF_TIMER_SHIFT      1
#define ETACON4_VRECHG_MASK             0x01
#define ETACON4_VRECHG_SHIFT            0

/* ETACON5 */
#define ETACON5_EN_TERM_MASK            0x01
#define ETACON5_EN_TERM_SHIFT           7
#define ETACON5_TSHIPM_ENTSEL_MASK      0x01
#define ETACON5_TSHIPM_ENTSEL_SHIFT     6
#define ETACON5_WATCHDOG_MASK           0x03
#define ETACON5_WATCHDOG_SHIFT          4
#define ETACON5_EN_TIMER_MASK           0x01
#define ETACON5_EN_TIMER_SHIFT          3
#define ETACON5_CHG_TIMER_MASK          0x01
#define ETACON5_CHG_TIMER_SHIFT         2
#define ETACON5_TREG_MASK               0x01
#define ETACON5_TREG_SHIFT              1
#define ETACON5_JEITA_VSET_MASK         0x01
#define ETACON5_JEITA_VSET_SHIFT        0

/* ETACON6 */
#define ETACON6_OVP_MASK                0x03
#define ETACON6_OVP_SHIFT               6
#define ETACON6_BOOSTV_MASK             0x03
#define ETACON6_BOOSTV_SHIFT            4
#define ETACON6_VINDPM_MASK             0x0F
#define ETACON6_VINDPM_SHIFT            0

/* ETACON7 */
#define ETACON7_IINDET_EN_MASK          0x01
#define ETACON7_IINDET_EN_SHIFT         7
#define ETACON7_TMR2X_EN_MASK           0x01
#define ETACON7_TMR2X_EN_SHIFT          6
#define ETACON7_BATFET_DIS_MASK         0x01
#define ETACON7_BATFET_DIS_SHIFT        5
#define ETACON7_BATFET_RST_WVBUS_MASK   0x01
#define ETACON7_BATFET_RST_WVBUS_SHIFT  4
#define ETACON7_BATFET_DLY_MASK         0x01
#define ETACON7_BATFET_DLY_SHIFT        3
#define ETACON7_BATFET_RST_EN_MASK      0x01
#define ETACON7_BATFET_RST_EN_SHIFT     2
#define ETACON7_VDPM_BAT_TRACK_MASK     0x03
#define ETACON7_VDPM_BAT_TRACK_SHIFT    0

/* ETACON8 */
#define ETACON8_VBUS_STAT_MASK          0x07
#define ETACON8_VBUS_STAT_SHIFT         5
#define ETACON8_CHRG_STAT_MASK          0x03
#define ETACON8_CHRG_STAT_SHIFT         3
#define ETACON8_PG_STAT_MASK            0x01
#define ETACON8_PG_STAT_SHIFT           2
#define ETACON8_THERM_STAT_MASK         0x01
#define ETACON8_THERM_STAT_SHIFT        1
#define ETACON8_VSYS_STAT_MASK          0x01
#define ETACON8_VSYS_STAT_SHIFT         0

/* ETACON9 */
#define ETACON9_WATCHDOG_FAULT_MASK     0x01
#define ETACON9_WATCHDOG_FAULT_SHIFT    7

#define ETACON9_BOOST_FAULT_MASK        0x01
#define ETACON9_BOOST_FAULT_SHIFT       6

#define ETACON9_CHRG_FAULT_MASK         0x03
#define ETACON9_CHRG_FAULT_SHIFT        4

#define ETACON9_BAT_FAULT_MASK          0x01
#define ETACON9_BAT_FAULT_SHIFT         3

#define ETACON9_NTC_FAULT_MASK          0x07
#define ETACON9_NTC_FAULT_SHIFT         0

/* ETACONA */
#define ETACONA_VBUS_GD_MASK            0x01
#define ETACONA_VBUS_GD_SHIFT           7

#define ETACONA_VINDPM_STAT_MASK        0x01
#define ETACONA_VINDPM_STAT_SHIFT       6

#define ETACONA_IINDPM_STAT_MASK        0x01
#define ETACONA_IINDPM_STAT_SHIFT       5

#define ETACONA_BATSNS_STAT_MASK        0x01
#define ETACONA_BATSNS_STAT_SHIFT       4

#define ETACONA_TOPOFF_ACTIVE_MASK      0x01
#define ETACONA_TOPOFF_ACTIVE_SHIFT     3

#define ETACONA_ACOV_STAT_MASK          0x01
#define ETACONA_ACOV_STAT_SHIFT         2

#define ETACONA_VINDPM_INT_MASK_MASK    0x01
#define ETACONA_VINDPM_INT_MASK_SHIFT   1

#define ETACONA_IINDPM_INT_MASK_MASK    0x03
#define ETACONA_IINDPM_INT_MASK_SHIFT   0

/* ETACONB */
#define ETACONB_REG_RST_MASK            0x01
#define ETACONB_REG_RST_SHIFT           7

#define ETACONB_PIN_MASK                0x0F
#define ETACONB_PIN_SHIFT               3

#define ETACONB_ETA_PARTID_MASK         0x01
#define ETACONB_ETA_PARTID_SHIFT        2

#define ETACONB_DEV_REV_MASK            0x03
#define ETACONB_DEV_REV_SHIFT           0

/* ETACONC */
#define ETACONC_JEITA_COOL_ISET_MASK    0x03
#define ETACONC_JEITA_COOL_ISET_SHIFT   6

#define ETACONC_JEITA_WARM_ISET_MASK    0x03
#define ETACONC_JEITA_WARM_ISET_SHIFT   4

#define ETACONC_JEITA_VT2_MASK          0x03
#define ETACONC_JEITA_VT2_SHIFT         2

#define ETACONC_JEITA_VT3_MASK          0x03
#define ETACONC_JEITA_VT3_SHIFT         0

#endif