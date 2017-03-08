/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef ANDROID

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"


// this is mod1_rst* which resets the first module (ASAC radio) when active low
//#define TDA7_MOD1_RST_LOW	IMX_GPIO_NR(4, 11) -->(4-1)*32 +11=107
// this is mod1_on* which powers ON the first module (ASAC radio) when active low
//#define TDA7_MOD1_ON_LOW	IMX_GPIO_NR(4, 10) -->(4-1)*32 +10=106

// the active low pins: they are now defined using properties in init.rc
//#define THE_RESET_PIN "/sys/class/gpio/gpio107/value"
//#define THE_ON_PIN    "/sys/class/gpio/gpio106/value"

// the name of the properties where the radio RF pins are located
#define PROPERTY_RF_ON_PIN "ro.radioRF.pinON"
#define PROPERTY_RF_RESET_PIN "ro.radioRF.pinReset"
#define PROPERTY_RF_COM_PORT "ro.radioRF.COMport"
#define PROPERTY_ASACZ_CC2650_BOOT_ENABLE "ro.ASACZ.CC2650_BOOT_ENABLE"


//LEDs
#define PROPERTY_YEL_SW 			"ro.radioRF.YEL_SW"
#define PROPERTY_EN_YEL_SW 			"ro.radioRF.EN_YEL_SW"
#define PROPERTY_RED_SW 			"ro.radioRF.RED_SW"
#define PROPERTY_EN_RED_SW 			"ro.radioRF.EN_RED_SW"

#define PROPERTY_EN_LED_LOW 		"ro.radioRF.EN_LED_LOW"
#define PROPERTY_LED_RAD_GREEN_LOW 	"ro.radioRF.LED_RAD_GREEN_LOW"
#define PROPERTY_LED_BLU_SW_LOW 	"ro.radioRF.LED_BLU_SW_LOW"

// the names of the pins in the Linux sysfs
char rf_on_pin_value[PROPERTY_VALUE_MAX];
char rf_reset_pin_value[PROPERTY_VALUE_MAX];
char CC2650_BOOT_ENABLE_pin_value[PROPERTY_VALUE_MAX];


// defines to set the correct value to the ON pin
// it is active low
#define def_on_pin_active_value 1
#define def_on_pin_not_active_value (!def_on_pin_active_value)

// defines to set the correct value to the RESET pin
// it is active low
#define def_reset_pin_active_value 0
#define def_reset_pin_not_active_value (!def_reset_pin_active_value)


// defines to set the correct value to the CC2650_BOOT_ENABLE pin
// it is active HIGH
#define def_CC2650_BOOT_ENABLE_pin_active_value 1
#define def_CC2650_BOOT_ENABLE_pin_not_active_value (!def_CC2650_BOOT_ENABLE_pin_active_value)

// useful routine to get a RF pin name
// returns 1 if OK, -1 if name not found
int i_get_rf_pin_name(const char *src, char *dst, int dst_length){
	char property_value[PROPERTY_VALUE_MAX];
	if (property_get(src, property_value, "")<=0){
		ALOGW("Could not get property [%s]", src);		
		return -1;
	}
	strncpy(dst,property_value,dst_length);
	//ALOGW("Property [%s] found value [%s], copied is [%s]", src, property_value,dst);		
	return 1;
}

// returns 0 if radio not exists, else returns 1
int radio_asac_barebone_exists()
{
    int fd;
    // if RF on pin do not exists, exit with 0 because radio do not exists
	if (i_get_rf_pin_name(PROPERTY_RF_ON_PIN, rf_on_pin_value, sizeof(rf_on_pin_value))<0){
		return 0;
	}

    fd = open(rf_on_pin_value, O_RDWR);
    if(fd < 0){
		ALOGW("Unable to open pin [%s], retcode is %i", rf_on_pin_value, fd);		
        return 0;
    }
    close(fd);
	//ALOGW("Radio ASAC exists OK");		
    return 1;
}

// write a value on a radio RF pin
// returns 0 if pin set was done OK, else -1
static int pin_set_value(char *pin_name, int value)
{
    int nwr, ret, fd;
    char value_str[20];

    fd = open(pin_name, O_RDWR);
    if(fd < 0){
		ALOGW("Unable to open pin [%s], retcode is %i", pin_name, fd);		
        return errno;
    }

    nwr = sprintf(value_str, "%d\n", value);
    ret = write(fd, value_str, nwr);

    close(fd);

    return (ret == nwr) ? 0 : -1;
}
//#define def_reset_tx_dtr_rts
void enable_radio_RX_pin(int enable_RX_mode);

// power ON the radio (and deactivate the reset)
int radio_asac_barebone_on()
{
    int i_ret_value[2];
#ifdef def_reset_tx_dtr_rts
	{
		char rf_comport_name[64];
		if (i_get_rf_pin_name(PROPERTY_RF_COM_PORT, rf_comport_name, sizeof(rf_comport_name)) < 0)
		{
			ALOGW("Unable to get port name %s", PROPERTY_RF_COM_PORT);
		}
		else
		{
			int fd = open (rf_comport_name, O_RDWR | O_NOCTTY | O_SYNC);
			if (fd < 0)
			{
				ALOGW("Unable to open port %s[%s]", PROPERTY_RF_COM_PORT, rf_comport_name);		
			}
			else
			{
				// reset the break
				int error_ioctl = ioctl(fd, TIOCCBRK);
				if (error_ioctl)
				{
					ALOGW("Unable to clear the break!");		
				}
				// set both RTS and CTS
				int bits = TIOCM_RTS | TIOCM_CTS;
				error_ioctl = ioctl(fd, TIOCMBIS, &bits);
				if (error_ioctl)
				{
					ALOGW("Unable to set the RTS and CTS!");		
				}
				else
				{
					ALOGW("OK set RTS and CTS!");		
				}
				// close the serial port
				if (close(fd))
				{
					ALOGW("Unable to close the serial port!");		
				}
			}
		}

	}

#endif
    // get the correct names for the pins
	if (i_get_rf_pin_name(PROPERTY_RF_ON_PIN, rf_on_pin_value, sizeof(rf_on_pin_value))<0){
		return -1;
	}
	if (i_get_rf_pin_name(PROPERTY_RF_RESET_PIN, rf_reset_pin_value, sizeof(rf_reset_pin_value))<0){
		return -1;
	}
	// activate the reset
    i_ret_value[0]=pin_set_value(rf_reset_pin_value, def_reset_pin_active_value);
    // switch ON the radio
    i_ret_value[1]=pin_set_value(rf_on_pin_value, def_on_pin_active_value);
    // 50 ms delay
    usleep(50*1000);
    // deactivate the reset
    i_ret_value[0]=pin_set_value(rf_reset_pin_value, def_reset_pin_not_active_value);
    if (i_ret_value[0]<0)
    {
        return i_ret_value[0];
    }
    return i_ret_value[1];
}

// power off the radio
int radio_asac_barebone_off()
{
#ifdef def_reset_tx_dtr_rts
	{
		if (i_get_rf_pin_name(PROPERTY_RF_RESET_PIN, rf_reset_pin_value, sizeof(rf_reset_pin_value)) < 0)
		{
			ALOGW("Unable to get reset pin name %s", PROPERTY_RF_RESET_PIN);
		}
		else
		{
			// activate the reset = set reset to zero = remove power from the reset line
		    	pin_set_value(rf_reset_pin_value, def_reset_pin_active_value);
			ALOGW("Reset pin forced to 0V");
		}

		char rf_comport_name[64];
		ALOGW("About to shut off TX, RTS, CTS");
		if (i_get_rf_pin_name(PROPERTY_RF_COM_PORT, rf_comport_name, sizeof(rf_comport_name)) < 0)
		{
			ALOGW("Unable to get port name %s", PROPERTY_RF_COM_PORT);
		}
		else
		{
			int fd = open (rf_comport_name, O_RDWR | O_NOCTTY | O_SYNC);
			if (fd < 0)
			{
				ALOGW("Unable to open port %s[%s]", PROPERTY_RF_COM_PORT, rf_comport_name);		
			}
			else
			{
				ALOGW("OK open port %s[%s]", PROPERTY_RF_COM_PORT, rf_comport_name);
				// set the break
				int error_ioctl = ioctl(fd, TIOCSBRK);
				if (error_ioctl)
				{
					ALOGW("Unable to set the break!");		
				}
				else
				{
					ALOGW("OK set break!");		
				}
				// reset both RTS and CTS
				int bits = TIOCM_RTS | TIOCM_CTS;
				error_ioctl = ioctl(fd, TIOCMBIC, &bits);
				if (error_ioctl)
				{
					ALOGW("Unable to clear the RTS and CTS!");		
				}
				else
				{
					ALOGW("OK reset RTS and CTS!");		
				}
				if (close(fd))
				{
					ALOGW("Unable to close the serial port!");		
				}
			}
		}
	}

#endif
	// get the name for the ON pin
	if (i_get_rf_pin_name(PROPERTY_RF_ON_PIN, rf_on_pin_value, sizeof(rf_on_pin_value))<0){
		return -1;
	}
    return pin_set_value(rf_on_pin_value, def_on_pin_not_active_value);
}

// min 10ms max 1000ms reset activated/deactivated
int radio_asac_barebone_reset_cycle(int i_reset_duration_ms)
{
    int i_ret_value[2];
    if (i_reset_duration_ms<1){
        i_reset_duration_ms=10;
    }
    else if (i_reset_duration_ms>1000){
	i_reset_duration_ms=1000;
    }
    // get the RF reset pin name
	if (i_get_rf_pin_name(PROPERTY_RF_RESET_PIN, rf_reset_pin_value, sizeof(rf_reset_pin_value))<0){
		return -1;
	}
    i_ret_value[0]=pin_set_value(rf_reset_pin_value, def_reset_pin_active_value);
// wait only if the reset activation was successful
    if (i_ret_value[0]>=0){
		usleep(i_reset_duration_ms*1000);
    }
    i_ret_value[1]=pin_set_value(rf_reset_pin_value, def_reset_pin_not_active_value);
    if (i_ret_value[0]<0){
        return i_ret_value[0];
    }
    return i_ret_value[1];
}



// blu LED on/off
// returns:
// 0 if all OK
// -1 if error
int blue_LED_set(int off0_on1)
{
    int i_ret_value[2];
	char en_led_low_value[PROPERTY_VALUE_MAX];
	char led_blu_sw_low_value[PROPERTY_VALUE_MAX];
    
    // get the correct names for the pins
	if (i_get_rf_pin_name(PROPERTY_EN_LED_LOW, en_led_low_value, sizeof(en_led_low_value))<0)
	{
		return -1;
	}
	if (i_get_rf_pin_name(PROPERTY_LED_BLU_SW_LOW, led_blu_sw_low_value, sizeof(led_blu_sw_low_value))<0)
	{
		return -1;
	}
	// set EN_LED_LOW = 0 so we can drive the BLU LED
    i_ret_value[0]=pin_set_value(en_led_low_value, 0);
    // set the BLU LED ff/ON; if requested off, I write 1, if requested on, I write 0
    i_ret_value[1]=pin_set_value(led_blu_sw_low_value, !off0_on1);
    if (i_ret_value[0]<0)
    {
        return i_ret_value[0];
    }
    return i_ret_value[1];
}


// green LED on/off
// returns:
// 0 if all OK
// -1 if error
int green_LED_set(int off0_on1)
{
    int i_ret_value[2];
	char en_led_low_value[PROPERTY_VALUE_MAX];
	char led_green_sw_low_value[PROPERTY_VALUE_MAX];
    
    // get the correct names for the pins
	if (i_get_rf_pin_name(PROPERTY_EN_LED_LOW, en_led_low_value, sizeof(en_led_low_value))<0)
	{
		return -1;
	}
	if (i_get_rf_pin_name(PROPERTY_LED_RAD_GREEN_LOW, led_green_sw_low_value, sizeof(led_green_sw_low_value))<0)
	{
		return -1;
	}
	// set EN_LED_LOW = 0 so we can drive the BLU LED
    i_ret_value[0]=pin_set_value(en_led_low_value, 0);
    // set the GREEN LED ff/ON; if requested off, I write 1, if requested on, I write 0
    i_ret_value[1]=pin_set_value(led_green_sw_low_value, !off0_on1);
    if (i_ret_value[0]<0)
    {
        return i_ret_value[0];
    }
    return i_ret_value[1];
}

int is_OK_do_CC2650_reset(unsigned int enable_boot_mode)
{
	int is_OK = 1;

	// get the RF reset pin name
	if (is_OK)
	{
		if (i_get_rf_pin_name(PROPERTY_RF_RESET_PIN, rf_reset_pin_value, sizeof(rf_reset_pin_value)) < 0)
		{
			is_OK = 0;
		}
	}

	// get the CC2650_BOOT_ENABLE pin name
	if (is_OK)
	{

		if (i_get_rf_pin_name(PROPERTY_ASACZ_CC2650_BOOT_ENABLE, CC2650_BOOT_ENABLE_pin_value, sizeof(CC2650_BOOT_ENABLE_pin_value)) < 0)
		{
			is_OK = 0;
		}
	}

	// ACTIVATE THE RESET PIN
	if (is_OK)
	{
		ALOGI("RESET ACTIVE");
		if (pin_set_value(rf_reset_pin_value, def_reset_pin_active_value) < 0)
		{
			is_OK = 0;
		}
	}

	// wait 100ms
	if (is_OK)
	{
		usleep(100 * 1000);
	}

	// ACTIVATE/DEACTIVATE (depending upon the operation passed as parameter) THE BOOT ENABLE PIN
	if (is_OK)
	{
		if (enable_boot_mode)
		{
			ALOGI("ENABLING BOOT MODE");
		}
		else
		{
			ALOGI("DISABLING BOOT MODE");
		}
		int pin_value = enable_boot_mode ? def_CC2650_BOOT_ENABLE_pin_active_value : def_CC2650_BOOT_ENABLE_pin_not_active_value;
		if (pin_set_value(CC2650_BOOT_ENABLE_pin_value, pin_value) < 0)
		{
			is_OK = 0;
		}
	}

	// wait 200ms
	if (is_OK)
	{
		usleep(200 * 1000);
	}

	// DEACTIVATE THE RESET PIN
	if (is_OK)
	{
		ALOGI("RESET NOT ACTIVE");
		if (pin_set_value(rf_reset_pin_value, def_reset_pin_not_active_value) < 0)
		{
			is_OK = 0;
		}
	}

	// wait 10ms
	if (is_OK)
	{
		usleep(10 * 1000);
	}

	return is_OK;
}


#endif
