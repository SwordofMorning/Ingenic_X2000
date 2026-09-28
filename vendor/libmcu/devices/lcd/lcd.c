#include <stdio.h>
#include <device/lcd.h>

#ifdef APP_libmcu_device_x2600_lcd_fw050
extern int lcd_fw050_init(void);
#endif

#ifdef APP_libmcu_device_x2600_lcd_kx070
extern int lcd_kx070_init(void);
#endif

void lcd_init(void)
{
#ifdef APP_libmcu_device_x2600_lcd_fw050
    lcd_fw050_init();
#endif
#ifdef APP_libmcu_device_x2600_lcd_kx070
    lcd_kx070_init();
#endif
}
