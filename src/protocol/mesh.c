/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#include <protocol/mesh.h>
#include <protocol/led_config.h>
#include <protocol/rbc_mesh_common.h>

#include <common/cs_boards.h>
#include <drivers/serial.h>

void rbc_mesh_event_handler(rbc_mesh_event_t* evt)                                                                      
{                                                                                                                       
	TICK_PIN(28);                                                                                                       
	nrf_gpio_pin_toggle(PIN_GPIO_LED7);
	switch (evt->event_type)                                                                                            
	{                                                                                                                   
		case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:                                                                       
		case RBC_MESH_EVENT_TYPE_NEW_VAL:                                                                               
		case RBC_MESH_EVENT_TYPE_UPDATE_VAL:                                                                            

			if (evt->value_handle > 2)                                                                                  
				break; 
			if (evt->data[0]) {
				LOGi("Got data in: %i, %i", evt->value_handle, evt->data[0]);
			}    
			led_config(evt->value_handle, evt->data[0]);  
			break; 
	}                                                                                                                   
}        


