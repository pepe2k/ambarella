
#ifndef __IMG_DEV_H__
#define __IMG_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "devices.h"


/* dc_iris.c */
u32 dc_iris_is_supported(void);
int dc_iris_init(void);
int	dc_iris_deinit(void);
int dc_iris_enable(void);
int dc_iris_disable(void);
int dc_iris_update_duty(int duty_cycle);

/* ir_led.c */
u32 ir_led_is_supported(void);
s32 ir_led_init(u32 init);
s32 ir_led_get_brightness(void);
s32 ir_led_set_brightness(s32 brightness);
s32 ir_led_get_state(void);
s32 ir_led_set_state(u32 value);
s32 ir_led_get_adc_value(u32 * value);
u32 ir_cut_is_supported(void);
s32 ir_cut_init(u32 init);
s32 ir_cut_set_state(u32 value);
s32 ir_cut_get_state(void);

#ifdef __cplusplus
}
#endif

#endif //  __IMG_DEV_H__


