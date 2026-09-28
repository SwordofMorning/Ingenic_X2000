#include "ETA6918.h"
#include "iic.h"

void eta6918_read_interface(unsigned char RegNum,
				    unsigned char *val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char eta6918_reg = 0;
	I2C_ReadByte(&eta6918_reg,eta6918_IIC_ADDRESS,RegNum);
	eta6918_reg &= (MASK << SHIFT);
	*val = (eta6918_reg >> SHIFT);
}
void eta6918_config_interface(unsigned char RegNum,
				      unsigned char val, unsigned char MASK,
				      unsigned char SHIFT)
{
	unsigned char eta6918_reg = 0;
	unsigned char eta6918_reg_ori = 0;
	unsigned int ret = 0;

	I2C_ReadByte(&eta6918_reg,eta6918_IIC_ADDRESS,RegNum);
	eta6918_reg_ori = eta6918_reg;
	eta6918_reg &= ~(MASK << SHIFT);
	eta6918_reg |= (val << SHIFT);
	ret = I2C_WriteByte(eta6918_IIC_ADDRESS,RegNum, eta6918_reg);
}
/* CON0---------------------------------------------------- */
void eta6918_set_en_hiz(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_EN_HIZ_MASK),
				       (unsigned char) (CON0_EN_HIZ_SHIFT)
				      );
}

void eta6918_set_TS_IGNORE(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_TS_IGNORE_MASK),
				       (unsigned char) (CON0_TS_IGNORE_SHIFT)
				      );
}

void eta6918_set_iinlim(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_BATSNS_DIS_MASK),
				       (unsigned char) (CON0_BATSNS_DIS_SHIFT)
				      );
}



void eta6918_set_iinlim(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_IINLIM_MASK),
				       (unsigned char) (CON0_IINLIM_SHIFT)
				      );
}





/* CON1---------------------------------------------------- */

void eta6918_set_reg_rst(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON11),
				       (unsigned char) (val),
				       (unsigned char) (CON11_REG_RST_MASK),
				       (unsigned char) (CON11_REG_RST_SHIFT)
				      );
}

void eta6918_set_pfm(unsigned int val)
{
	unsigned char otg_en=0;
	eta6918_read_interface((unsigned char) (eta6918_CON1),
				     (&otg_en),
				     (unsigned char) (CON1_OTG_CONFIG_MASK),
				     (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				    );

	if (otg_en==1) //PFM_DIS=1 only for OTG
	{
		eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_PFM_MASK),
				       (unsigned char) (CON1_PFM_SHIFT)
				      );
	}
	else
	{
		eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (0),
				       (unsigned char) (CON1_PFM_MASK),
				       (unsigned char) (CON1_PFM_SHIFT)
				      );
	}
}
//feed watchdog
void eta6918_set_wdt_rst(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_WDT_RST_MASK),
				       (unsigned char) (CON1_WDT_RST_SHIFT)
				      );
}

void eta6918_set_otg_config(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_OTG_CONFIG_MASK),
				       (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				      );
}

unsigned int eta6918_get_otg_config(void)
{
	unsigned char val = 0;

	eta6918_read_interface((unsigned char) (eta6918_CON1),
				     (&val),
				     (unsigned char) (CON1_OTG_CONFIG_MASK),
				     (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				    );
	return val;
}


void eta6918_set_chg_config(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_CHG_CONFIG_MASK),
				       (unsigned char) (CON1_CHG_CONFIG_SHIFT)
				      );

}


void eta6918_set_sys_min(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_SYS_MIN_MASK),
				       (unsigned char) (CON1_SYS_MIN_SHIFT)
				      );
}

void eta6918_set_batlowv(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_MIN_VBAT_SEL_MASK),
				       (unsigned char) (CON1_MIN_VBAT_SEL_SHIFT)
				      );
}



/* CON2---------------------------------------------------- */
void eta6918_set_rdson(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_Q1_FULLON_MASK),
				       (unsigned char) (CON2_Q1_FULLON_SHIFT)
				      );
}


void eta6918_set_ichg(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_ICHG_MASK),
				       (unsigned char) (CON2_ICHG_SHIFT)
				      );
}



/* CON3---------------------------------------------------- */

void eta6918_set_iprechg(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_IPRECHG_MASK),
				       (unsigned char) (CON3_IPRECHG_SHIFT)
				      );
}

void eta6918_set_iterm(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_ITERM_MASK),
				       (unsigned char) (CON3_ITERM_SHIFT)
				      );
}

/* CON4---------------------------------------------------- */

void eta6918_set_vreg(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VREG_MASK),
				       (unsigned char) (CON4_VREG_SHIFT)
				      );
}

void eta6918_set_topoff_timer(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_TOPOFF_TIMER_MASK),
				       (unsigned char) (CON4_TOPOFF_TIMER_SHIFT)
				      );

}


void eta6918_set_vrechg(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VRECHG_MASK),
				       (unsigned char) (CON4_VRECHG_SHIFT)
				      );
}

/* CON5---------------------------------------------------- */

void eta6918_set_en_term(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TERM_MASK),
				       (unsigned char) (CON5_EN_TERM_SHIFT)
				      );
}

void eta6918_TSHIPM_ENTSEL(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_TSHIPM_ENTSEL_MASK),
				       (unsigned char) (CON5_TSHIPM_ENTSEL_SHIFT)
				      );
}


void eta6918_set_watchdog(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_WATCHDOG_MASK),
				       (unsigned char) (CON5_WATCHDOG_SHIFT)
				      );
}

void eta6918_set_en_timer(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TIMER_MASK),
				       (unsigned char) (CON5_EN_TIMER_SHIFT)
				      );
}

void eta6918_set_chg_timer(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_CHG_TIMER_MASK),
				       (unsigned char) (CON5_CHG_TIMER_SHIFT)
				      );
}

/* CON6---------------------------------------------------- */
void eta6918_set_ovp(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_OVP_MASK),
				       (unsigned char) (CON6_OVP_SHIFT)
				      );

}

void eta6918_set_vindpm(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_VINDPM_MASK),
				       (unsigned char) (CON6_VINDPM_SHIFT)
				      );
}


void eta6918_set_boostv(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BOOSTV_MASK),
				       (unsigned char) (CON6_BOOSTV_SHIFT)
				      );
}



/* CON7---------------------------------------------------- */

//ǿ��dpdm��һ��ʶ�� 6973�޴˹��� 6974 6963��
void eta6974_force_bc12(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_FORCE_DPDM_MASK),
				       (unsigned char) (CON7_FORCE_DPDM_SHIFT)
				      );
}



void eta6918_set_tmr2x_en(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON7),
					(unsigned char) (val),
					(unsigned char) (CON7_TMR2X_EN_MASK),
					(unsigned char) (CON7_TMR2X_EN_SHIFT)
					);
}
//ship mode �ر�Q4·����
void eta6918_set_batfet_disable(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_Disable_MASK),
				(unsigned char) (CON7_BATFET_Disable_SHIFT)
				);
}


void eta6918_set_batfet_delay(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_BATFET_DLY_MASK),
				       (unsigned char) (CON7_BATFET_DLY_SHIFT)
				      );
}

void eta6918_set_batfet_reset_enable(unsigned int val)
{
	eta6918_config_interface((unsigned char) (eta6918_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_RST_EN_MASK),
				(unsigned char) (CON7_BATFET_RST_EN_SHIFT)
				);
}


/* CON8---------------------------------------------------- */

unsigned int eta6918_get_system_status(void)
{
	unsigned char val = 0;

	eta6918_read_interface((unsigned char) (eta6918_CON8),
				     (&val), (unsigned char) (0xFF),
				     (unsigned char) (0x0)
				    );
	return val;
}

unsigned int eta6918_get_vbus_stat(void)
{
	unsigned char val = 0;

	eta6918_read_interface((unsigned char) (eta6918_CON8),
				     (&val),
				     (unsigned char) (CON8_VBUS_STAT_MASK),
				     (unsigned char) (CON8_VBUS_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta6918_get_chrg_stat(void)
{
	unsigned char val = 0;
	eta6918_read_interface((unsigned char) (eta6918_CON8),
				     (&val),
				     (unsigned char) (CON8_CHRG_STAT_MASK),
				     (unsigned char) (CON8_CHRG_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta6918_get_vsys_stat(void)
{
	unsigned char val = 0;

	eta6918_read_interface((unsigned char) (eta6918_CON8),
				     (&val),
				     (unsigned char) (CON8_VSYS_STAT_MASK),
				     (unsigned char) (CON8_VSYS_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta6918_get_pg_stat(void)
{
	unsigned char val = 0;

	eta6918_read_interface((unsigned char) (eta6918_CON8),
				     (&val),
				     (unsigned char) (CON8_PG_STAT_MASK),
				     (unsigned char) (CON8_PG_STAT_SHIFT)
				    );
	return val;
}

/*CON10----------------------------------------------------------*/

void eta6918_set_int_mask(unsigned int val)
{

	eta6918_config_interface((unsigned char) (eta6918_CON10),
				       (unsigned char) (val),
				       (unsigned char) (CON10_INT_MASK_MASK),
				       (unsigned char) (CON10_INT_MASK_SHIFT)
				      );
}

/*********************************************************

void eta6918_dump_register()
{

		unsigned char i = 0;
		for (i = 0; i < eta6918_REG_NUM; i++) {
			eta6918_read_byte(i, &eta6918_reg[i]);
			printf("[0x%x]=0x%x ", i, eta6918_reg[i]);
		}
		printf("\n");
}
 *********************************************************/

static unsigned int bmt_find_closest_level(const unsigned int *pList,
		unsigned int number,
		unsigned int level)
{
	unsigned int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = 1;
	else
		max_value_in_last_element = 0;

	if (max_value_in_last_element == 1)
	{
		for (i = (number - 1); i != 0;
		     i--) {
		/* max value in the last element */
			if (pList[i] <= level) {
				return pList[i];
			}
		}
		return pList[0];
		/* return CHARGE_CURRENT_0_00_MA; */
	}
	else
	{
		/* max value in the first element */
		for (i = 0; i < number; i++)
		{
			if (pList[i] <= level)
				return pList[i];
		}
		return pList[number - 1];
	}
}


unsigned int charging_parameter_to_value(const unsigned int
		*parameter, const unsigned int array_size,
		const unsigned int val)
{
	unsigned int i;
	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

}




/********************function*****************************/

//��ȡ������
void eta6918_get_current(u32 *ichg)
{
	unsigned char ret_val = 0;
	/* Get current level */
	eta6918_read_interface(eta6918_CON2, &ret_val, CON2_ICHG_MASK,
			       CON2_ICHG_SHIFT);
	/* Parsing */
	*ichg = (ret_val * 60);

}
//���ó�����
void eta6918_set_current(u32 current_value)
{
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;
	current_value=current_value/10;
	array_size = sizeof(CS_VTH)/sizeof(CS_VTH[0]);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(CS_VTH, array_size,
			 set_chr_current);
	eta6918_set_ichg(register_value);

}
//��ȡ��������
void eta6918_get_input_current(u32 *aicr)
{
	unsigned char val = 0;
	eta6918_read_interface(eta6918_CON0, &val, CON0_IINLIM_MASK,
			       CON0_IINLIM_SHIFT);
	*aicr = INPUT_CS_VTH[val];
}
//������������
void eta6918_set_input_current(u32 current_value)
{
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;
	array_size = sizeof(INPUT_CS_VTH)/sizeof(INPUT_CS_VTH[0]);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size,
			 set_chr_current);
	eta6918_set_iinlim(register_value);
}
//ι��
void eta6918_reset_watch_dog_timer()
{
	eta6918_set_wdt_rst(0x1);	/* Kick watchdog */
	eta6918_set_watchdog(0x00);	/*disable watchdog */
}
//����������ѹ
void eta6918_set_vindpm_voltage(u32 vindpm)
{
	unsigned int register_value;
	unsigned int array_size;

	array_size = sizeof(VINDPM_REG)/sizeof(VINDPM_REG[0]);
	vindpm = bmt_find_closest_level(VINDPM_REG, array_size, vindpm);
	register_value = charging_parameter_to_value(VINDPM_REG, array_size, vindpm);
	eta6918_set_vindpm(register_value);
}
void eta6918_set_voltage(u32 voreg)
{
	unsigned int register_value;
	unsigned int array_size;

	array_size = sizeof(VBAT_CV_VTH)/sizeof(VBAT_CV_VTH[0]);
	voreg = bmt_find_closest_level(VBAT_CV_VTH, array_size, voreg);
	register_value = charging_parameter_to_value(VBAT_CV_VTH, array_size, voreg);
	eta6918_set_vreg(register_value);
}



//��ȡ���״̬ 08�Ĵ���
static int eta6918_get_charging_status( int *is_done)
{
	unsigned int status = 1;
	unsigned int ret_val;

	ret_val = eta6918_get_chrg_stat();

	if (ret_val == 0x3)
		*is_done = 1;
	else
		*is_done = 0;

	return status;
}
//ʹ�� otg
void eta6918_enable_otg( int en)
{
	if (en) {
		eta6918_set_chg_config(0);
		eta6918_set_otg_config(1);
	} else {
		eta6918_set_otg_config(0);
		eta6918_set_chg_config(1);
	}
}
void eta6918_set_boost_current_limit( u32 uA)
{
	u32 array_size = 0;
	u32 boost_ilimit = 0;
	u8 boost_reg = 0;
	uA /= 1000;

	array_size = sizeof(BOOST_CURRENT_LIMIT)/sizeof(BOOST_CURRENT_LIMIT[0]);
	boost_ilimit = bmt_find_closest_level(BOOST_CURRENT_LIMIT, array_size,
					      uA);
	boost_reg = charging_parameter_to_value(BOOST_CURRENT_LIMIT,
						array_size, boost_ilimit);
	eta6918_set_boost_lim(boost_reg);
}

void eta6918_enable_safetytimer(int en)
{
	if (en==1)
		eta6918_set_en_timer(0x1);
	else
		eta6918_set_en_timer(0x0);
}

void charging_hw_init(void)
{
/**************test**************/
	eta6918_set_en_hiz(0x0);
	eta6918_set_wdt_rst(0x1);	/* Kick watchdog */
	eta6918_set_watchdog(0);
	eta6918_set_input_current(3000000);
	eta6918_set_current(2500000);
	eta6918_set_voltage(4360000);
	eta6918_set_en_term(0x1);	/* Enable termination */
	eta6918_set_watchdog(0x0);	/* WDT disable */


}


