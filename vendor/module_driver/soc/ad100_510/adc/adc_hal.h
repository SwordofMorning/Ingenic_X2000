#ifndef _SOC_ADC_HAL_H_
#define _SOC_ADC_HAL_H_

void adc_hal_power_on_phy(void);

void adc_hal_power_off_phy(void);

void adc_hal_set_seq0_channels(unsigned char *channels, int len);

void adc_hal_set_seq0_CR_register(int channel_cnt, int enable_channel_num);

void adc_hal_set_seq1_continus_clk_div(unsigned int div);

void adc_hal_set_seq1_delay_clk_div(unsigned int div);

void adc_hal_set_seq1_group_mode(
    unsigned char group_len, unsigned char *lens, unsigned short *delays);

void adc_hal_set_seq1_delay(unsigned short *delays, int len);

void adc_hal_set_seq1_channels(unsigned char *channels, int len);

void adc_hal_set_seq1_CR_register(
    enum adc_trigger_type trigger, int channel_cnt, int enable_channel_num);

void adc_hal_set_seq2_delay_clk_div(unsigned int div);

unsigned int seq2_delay(unsigned char *delays, int len);

void adc_hal_set_seq2_delay(unsigned char *delays, int len);

void adc_hal_set_seq2_channels(unsigned char *channels, int len);

void adc_hal_set_seq2_CR_register(
    enum adc_trigger_type trigger, int channel_cnt, int enable_channel_num);

#endif // _SOC_ADC_HAL_H_