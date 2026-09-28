#ifndef __ETA6918_H__
#define __ETA6918_H__
#include "stm32f10x.h"
#define  eta6918_IIC_ADDRESS 0x6A
/*eta6918 REG04 VREG[5:0]*/ /* set_voltage电池电压设置 */
static const unsigned int VBAT_CV_VTH[] = {
	3504000,3600000,3696000,3800000,
	3904000,4000000,4100000,4150000,
	4200000,4300000,4310000,4320000,
	4330000,4340000,4350000,4360000,
	4370000,4380000,4390000,4400000,
	4410000,4420000,4430000,4440000,
	4450000,4460000,4470000,4480000,
	4490000,4500000

};
/*eta6918 REG02 ICHG[5:0]*/ /* set_current 充电电流调节 */
static const unsigned int CS_VTH[] = {
	20000,40000,60000,80000,100000,
	120000,140000,160000,180000,200000,
	220000,240000,260000,280000,300000,
	320000,340000,360000,380000,400000,
	420000,440000,460000,480000,500000,
	520000,540000,560000,580000,600000,
	620000,640000,660000,680000,700000,
	720000,740000,760000,780000,800000,
	820000,840000,860000,880000,900000,
	920000,940000,960000,980000,1000000,
	1020000,1040000,1060000,1080000,1100000,
	1120000,1140000,1160000,1180000,
	1290000,1360000,1430000,1500000
};
/*eta6918 REG00 IINLIM[4:0]*/ /* set_input_current输入电流调节 */
static const unsigned int INPUT_CS_VTH[] = {
	100000,200000,300000,400000,
	500000,600000,700000,800000,
	900000,1000000,1100000,1200000,
	1300000,1400000,1500000,1600000,
	1700000,1800000,1900000,2000000,
	2100000,2200000,2300000,2400000,
	2500000,2600000,2700000,2800000,
	2900000,3000000,3100000,3200000,
};
/*eta6918 REG06 VINDPM[3:0]*/ /* set_vindpm_voltage输入电压调节 */
static const unsigned int VINDPM_REG[] = {
	3900, 4000, 4100, 4200, 4300, 4400,
	4500, 4600, 4700, 4800, 4900, 5000,
	5100, 5200, 5300, 5400
};
/*eta6918 REG0A BOOST_LIM[2:0], mA */ /* set_boost_current_limit */
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500, 1200
};




#define eta6918_CON0      					0x00
#define eta6918_CON1      					0x01
#define eta6918_CON2      					0x02
#define eta6918_CON3      					0x03
#define eta6918_CON4      					0x04
#define eta6918_CON5      					0x05
#define eta6918_CON6      					0x06
#define eta6918_CON7      					0x07
#define eta6918_CON8      					0x08
#define eta6918_CON9      					0x09
#define eta6918_CON10      					0x0A
#define	eta6918_CON11      					0x0B
#define eta6918_REG_NUM      				12

//--------------------CON0---------------
#define CON0_EN_HIZ_MASK      				0x01
#define CON0_EN_HIZ_SHIFT  					7
#define	CON0_TS_IGNORE_MASK					0x01
#define	CON0_TS_IGNORE_SHIFT 				6
#define	CON0_BATSNS_DIS_MASK				0x01
#define	CON0_BATSNS_DIS_SHIFT 				5
#define CON0_IINLIM_MASK   					0x1F
#define CON0_IINLIM_SHIFT  					0

//--------------------CON1---------------
#define CON1_PFM_MASK     					0x01
#define CON1_PFM_SHIFT    					7
#define CON1_WDT_RST_MASK  				  	0x01
#define CON1_WDT_RST_SHIFT    				6
#define CON1_OTG_CONFIG_MASK				0x01
#define CON1_OTG_CONFIG_SHIFT				5
#define CON1_CHG_CONFIG_MASK        		0x01
#define CON1_CHG_CONFIG_SHIFT       		4
#define CON1_SYS_MIN_MASK        			0x07
#define CON1_SYS_MIN_SHIFT       			1
#define	CON1_MIN_VBAT_SEL_MASK				0x01
#define	CON1_MIN_VBAT_SEL_SHIFT				0

//--------------------CON2---------------
#define	CON2_Q1_FULLON_MASK					0x01
#define	CON2_Q1_FULLON_SHIFT				6

#define CON2_ICHG_MASK   					0x3F
#define CON2_ICHG_SHIFT   					0

//--------------------CON3---------------
#define CON3_IPRECHG_MASK   				0x0F
#define CON3_IPRECHG_SHIFT  				4

#define CON3_ITERM_MASK           			0x0F
#define CON3_ITERM_SHIFT         			0

//--------------------CON4---------------
#define CON4_VREG_MASK     					0x1F
#define CON4_VREG_SHIFT    					3

#define	CON4_TOPOFF_TIMER_MASK 				0x03
#define	CON4_TOPOFF_TIMER_SHIFT 			1

#define CON4_VRECHG_MASK    				0x01
#define CON4_VRECHG_SHIFT   				0

//--------------------CON5---------------
#define CON5_EN_TERM_MASK      				0x01
#define CON5_EN_TERM_SHIFT     				7

#define CON5_TSHIPM_ENTSEL_MASK     		0x01
#define CON5_TSHIPM_ENTSEL_SHIFT    		6

#define CON5_WATCHDOG_MASK     				0x03
#define CON5_WATCHDOG_SHIFT    				4

#define CON5_EN_TIMER_MASK      			0x01
#define CON5_EN_TIMER_SHIFT     			3

#define CON5_CHG_TIMER_MASK         		0x01
#define CON5_CHG_TIMER_SHIFT        		2

#define CON5_TREG_MASK     					0x01
#define CON5_TREG_SHIFT    					1

#define CON5_JEITA_VSET_MASK     			0x01
#define CON5_JEITA_VSET_SHIFT    			0
//--------------------CON6---------------
#define	CON6_OVP_MASK						0x03
#define	CON6_OVP_SHIFT						6

#define	CON6_BOOSTV_MASK					0x3
#define	CON6_BOOSTV_SHIFT					4

#define	CON6_VINDPM_MASK					0x0F
#define	CON6_VINDPM_SHIFT					0

//--------------------CON7---------------
#define	CON7_FORCE_DPDM_MASK				0x01
#define	CON7_FORCE_DPDM_SHIFT				7

#define CON7_TMR2X_EN_MASK      			0x01
#define CON7_TMR2X_EN_SHIFT     			6

#define CON7_BATFET_Disable_MASK    		0x01
#define CON7_BATFET_Disable_SHIFT   		5

#define	CON7_BATFET_RST_WVBUS_MASK			0x01
#define	CON7_BATFET_RST_WVBUS_SHIFT			4

#define	CON7_BATFET_DLY_MASK				0x01
#define	CON7_BATFET_DLY_SHIFT				3

#define	CON7_BATFET_RST_EN_MASK				0x01
#define	CON7_BATFET_RST_EN_SHIFT			2

#define	CON7_VDPM_BAT_TRACK_MASK			0x03
#define	CON7_VDPM_BAT_TRACK_SHIFT			0

//--------------------CON8---------------
#define CON8_VBUS_STAT_MASK      			0x07
#define CON8_VBUS_STAT_SHIFT     			5

#define CON8_CHRG_STAT_MASK         		0x03
#define CON8_CHRG_STAT_SHIFT        		3

#define CON8_PG_STAT_MASK           		0x01
#define CON8_PG_STAT_SHIFT          		2

#define CON8_THERM_STAT_MASK        		0x01
#define CON8_THERM_STAT_SHIFT       		1

#define CON8_VSYS_STAT_MASK         		0x01
#define CON8_VSYS_STAT_SHIFT        		0

//--------------------CON9---------------
#define CON9_WATCHDOG_FAULT_MASK    		0x01
#define CON9_WATCHDOG_FAULT_SHIFT  			7

#define CON9_OTG_FAULT_MASK         		0x01
#define CON9_OTG_FAULT_SHIFT        		6

#define CON9_CHRG_FAULT_MASK        		0x03
#define CON9_CHRG_FAULT_SHIFT       		4

#define CON9_BAT_FAULT_MASK         		0x01
#define CON9_BAT_FAULT_SHIFT        		3

#define CON9_NTC_FAULT_MASK         		0x07
#define CON9_NTC_FAULT_SHIFT        		0

//--------------------CON10---------------
#define	CON10_VBUS_GD_MASK					0x01
#define	CON10_VBUS_GD_SHIFT					7

#define	CON10_VINDPM_STAT_MASK				0x01
#define	CON10_VINDPM_STAT_SHIFT				6

#define	CON10_IINDPM_STAT_MASK				0x01
#define	CON10_IINDPM_STAT_SHIFT				5

#define	CON10_TOPOFF_ACTIVE_MASK			0x01
#define	CON10_TOPOFF_ACTIVE_SHIFT			3

#define	CON10_ACOV_STAT_MASK				0x01
#define	CON10_ACOV_STAT_SHIFT				2

#define	CON10_VINDPM_INT_MASK				0x01
#define	CON10_VINDPM_INT_SHIFT				1

#define	CON10_INT_MASK_MASK					0x03
#define	CON10_INT_MASK_SHIFT				0

//--------------------CON11---------------
#define CON11_REG_RST_MASK     				0x01
#define CON11_REG_RST_SHIFT    				7

#define CON11_PN_MASK						0x0F
#define CON11_PN_SHIFT						3

#define CON11_Rev_MASK           			0x03
#define CON11_Rev_SHIFT          			0

//--------------------CON12---------------
#define CON11_JEITA_COOL_ISET_MASK     		0x03
#define CON11_JEITA_COOL_ISET_SHIFT    		6

#define CON11_JEITA_WARM_ISET_MASK     		0x03
#define CON11_JEITA_WARM_ISET_SHIFT    		4

#define CON11_JEITA_VT2_MASK     			0x03
#define CON11_JEITA_VT2_SHIFT    			2

#define CON11_JEITA_VT3_MASK     			0x03
#define CON11_JEITA_VT3_SHIFT    			0


//------------------------------------------
#define eta6918_CHG_STATUS_READY    		1
#define eta6918_CHG_STATUS_PROGRESS 		2
#define eta6918_CHG_STATUS_DONE     		3
#define eta6918_CHG_STATUS_NOT_CHARGING    	0








void eta6918_read_interface(unsigned char RegNum,
				    unsigned char *val, unsigned char MASK,
				    unsigned char SHIFT);

void eta6918_config_interface(unsigned char RegNum,
				      unsigned char val, unsigned char MASK,
				      unsigned char SHIFT);

void eta6918_set_en_hiz(unsigned int val);
void eta6918_set_iinlim(unsigned int val);
void eta6918_set_stat_ctrl(unsigned int val);
void eta6918_set_reg_rst(unsigned int val);
void eta6918_set_pfm(unsigned int val);
void eta6918_set_wdt_rst(unsigned int val);
void eta6918_set_otg_config(unsigned int val);
unsigned int eta6918_get_otg_config(void);
void eta6918_set_chg_config(unsigned int val);
void eta6918_set_sys_min(unsigned int val);
void eta6918_set_batlowv(unsigned int val);
void eta6918_set_rdson(unsigned int val);
void eta6918_set_boost_lim(unsigned int val);
void eta6918_set_ichg(unsigned int val);
void eta6918_set_iprechg(unsigned int val);
void eta6918_set_iterm(unsigned int val);
void eta6918_set_vreg(unsigned int val);
void eta6918_set_topoff_timer(unsigned int val);
void eta6918_set_vrechg(unsigned int val);
void eta6918_set_en_term(unsigned int val);
void eta6918_set_watchdog(unsigned int val);
void eta6918_set_en_timer(unsigned int val);
void eta6918_set_chg_timer(unsigned int val);
void eta6918_set_ovp(unsigned int val);
void eta6918_set_vindpm(unsigned int val);
void eta6918_set_boostv(unsigned int val);
void eta6974_force_bc12(unsigned int val);
void eta6918_set_tmr2x_en(unsigned int val);
void eta6918_set_batfet_disable(unsigned int val);
void eta6918_set_batfet_delay(unsigned int val);
void eta6918_set_batfet_reset_enable(unsigned int val);
unsigned int eta6918_get_system_status(void);
unsigned int eta6918_get_vbus_stat(void);
unsigned int eta6918_get_chrg_stat(void);
unsigned int eta6918_get_vsys_stat(void);
unsigned int eta6918_get_pg_stat(void);
void eta6918_set_int_mask(unsigned int val);
void eta6918_get_current(u32 *ichg);
void eta6918_set_current(u32 current_value);
void eta696x_get_input_current(u32 *aicr);
void eta6918_set_input_current(u32 current_value);
void eta6918_reset_watch_dog_timer();
void eta6918_set_vindpm_voltage(u32 vindpm);
static int eta696x_get_charging_status( int *is_done);
void eta6918_enable_otg( int en);
void eta6918_set_boost_current_limit( u32 uA);
void eta6918_enable_safetytimer(int en);
void charging_hw_init(void);
void eta6918_set_voltage(u32 voreg);
void charging_hw_init(void);



#endif