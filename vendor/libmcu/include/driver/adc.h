#ifndef _ADC_H_
#define _ADC_H_

#include <soc/adc.h>

int  adc_read_data(unsigned int channel);
void adc_init(void);
void adc_deinit(void);

#endif