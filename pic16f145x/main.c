/*
    CMSIS-DAP implementation for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013-2018 Peter Lawrence.

    based on top of M-Stack USB driver stack by Alan Ott, Signal 11 Software

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "usb.h"
#include <xc.h>
#include <string.h>
#include "usb_config.h"
#include "usb_ch9.h"
#include "usb_hid.h"
#include "dm.h"

/* 
since this is a downloaded app, configuration words (e.g. __CONFIG or #pragma config) are not relevant
*/

static uint8_t calc_next_index(uint8_t current)
{
	current++;

	if (current >= DAP_PACKET_COUNT)
		current = 0;

	return current;
}

int main(void)
{
	uint8_t *TxDataBuffer;
	uint8_t *RxDataBuffer;
	static uint8_t incoming[DAP_PACKET_COUNT][EP_1_OUT_LEN];
	uint8_t read_index, write_index, next_index, action_index;

	usb_init();

	TxDataBuffer = usb_get_in_buffer(1);

	read_index = write_index = action_index = 0;

	for (;;)
	{
#ifndef USB_USE_INTERRUPTS
		usb_service();
#endif

		/* if USB isn't configured, there is no point in proceeding further */
		if (!usb_is_configured())
			continue;

		next_index = calc_next_index(write_index);

		/* if we pass this test, we are committed to make the usb_arm_out_endpoint() call */
		if ( usb_out_endpoint_has_data(1) && (next_index != read_index))
		{
			/* obtain a pointer to the receive buffer and the length of data contained within it */
			usb_get_out_buffer(1, &RxDataBuffer);

			memcpy(incoming[write_index], RxDataBuffer, EP_1_OUT_LEN);
			write_index = next_index;

			/* re-arm the endpoint to receive the next EP1 OUT */
			usb_arm_out_endpoint(1);
		}

		if (read_index != action_index)
		{
			/* proceed further only when the IN endpoint is ready */
			if (!usb_in_endpoint_halted(1) && !usb_in_endpoint_busy(1))
			{
				memcpy(TxDataBuffer, incoming[read_index], EP_1_IN_LEN);

				/* send a response back to the PC */
				usb_send_in_buffer(1, EP_1_IN_LEN);

				read_index = calc_next_index(read_index);
			}
		}

		if (action_index != write_index)
		{
			RxDataBuffer = incoming[action_index];

			/* invoke Dapper Miser implementation */
			dap_handler(RxDataBuffer);

			/* update action_index to the next message */
			action_index = calc_next_index(action_index);
		}

	}
}

int8_t app_unknown_setup_request_callback(const struct setup_packet *setup)
{
	return process_hid_setup_request(setup);
}

void interrupt isr()
{
#ifdef USB_USE_INTERRUPTS
	usb_service();
#endif
}
