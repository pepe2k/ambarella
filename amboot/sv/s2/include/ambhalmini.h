#ifndef _AMBHALMINI_H_INCLUDED_
#define _AMBHALMINI_H_INCLUDED_

#include "ambhal.h"

amb_hal_success_t amb_mini_set_sd_clock_frequency (amb_clock_frequency_t amb_clock_frequency) ;

amb_clock_frequency_t amb_mini_get_sd_clock_frequency (void) ;

amb_hal_success_t amb_mini_set_uart_clock_frequency (amb_clock_frequency_t amb_clock_frequency) ;

amb_clock_frequency_t amb_mini_get_uart_clock_frequency (void) ;

amb_hal_success_t amb_mini_set_ssi_clock_frequency (amb_clock_frequency_t amb_clock_frequency) ;

amb_clock_frequency_t amb_mini_get_ssi_clock_frequency (void) ;

amb_hal_success_t amb_mini_set_ssi2_clock_frequency (amb_clock_frequency_t amb_clock_frequency) ;

amb_clock_frequency_t amb_mini_get_ssi2_clock_frequency (void) ;

amb_clock_frequency_t amb_mini_get_core_clock_frequency (void) ;

amb_hal_success_t amb_mini_reset_all (void) ;

amb_boot_type_t amb_mini_get_boot_type (void) ;

amb_system_configuration_t amb_mini_get_system_configuration (void) ;

#endif // ifndef _AMBHALMINI_H_INCLUDED_

