/*
    Dapper Miser: lightweight CMSIS-DAP implementation

    Copyright (C) 2013-2018 Peter Lawrence.

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

#include <string.h>
#include <stdint.h>
#include "swdio_bsp.h"
#include "dm_bsp.h"

/*
In approaching this code, it is important to understand that this 
was originally developed for the PIC16F145x microcontroller.

Said microcontroller is 8-bit and lacks a stack.

As a result, variables all deal with 8-bit quantities.  Parameters 
are primarily passed via shared variables rather than as function 
arguments.
*/

/*
Theory of operation:

an external device-specific USB Vendor HID implementation calls 
dap_handler() in this file.  This and additional support functions 
local to this file perform the CMSIS-DAP functionality.

Processor and board-specific access to GPIO pins is abstracted
away by the #include "swdio_bsp.h" above.
*/

static uint8_t out_count;
static uint8_t parity;
static uint8_t flags;

#define FLAG_TRANSFERBLOCK     0x01
#define FLAG_PIPELINEDAPREAD   0x02
#define FLAG_OMITREQUESTDECODE 0x04
#define FLAG_BUSFAULT          0x08
#define FLAG_WRITEABORT        0x10

/* shifts MIN(out_count,8) bits of data, LSB first */

static void shift_bits_out(uint8_t data)
{
	uint8_t bit_index;

	/* drive both clock and data */
	DATA_ENABLE;

	for (bit_index = 0; bit_index < 8; bit_index++)
	{
		CLK_LOW;
		if (data & 0x01)
		{
			DATA_HIGH;
			parity++;
		}
		else
		{
			DATA_LOW;
		}
		data >>= 1;
		CLK_HIGH;

		if (0 == --out_count)
			break;
	}

	CLK_LOW;
}

/* wrapper of shift_bits_out() to perform DAP_SWJ_Sequence */

static void swj_sequence(const uint8_t *pnt)
{
	uint8_t data;

	out_count = *pnt++;

	do
	{
		data = *pnt++;
		shift_bits_out(data);
	} while (out_count);
}

/* wrapper of shift_bits_out() to perform DAP_JTAG_Sequence */

#ifdef DAP_SUPPORT_JTAG_SEQUENCE
static void jtag_sequence(const uint8_t *pnt)
{
	/* 
	It *ought* to be possible to omit this, but certain IDE implementations
	assume the CMSIS-DAP implementation supports this, even when SWD-only.
	The CMSIS-DAP specification fails to formalize which messages are used.
	*/

	uint8_t sequence_count, data;

	sequence_count = *pnt++;

	while (sequence_count--)
	{
		data = (*pnt & 0x40) ? 0xFF : 0x00;
		out_count = *pnt++ & 0x3F;

		do
		{
			pnt++;
			shift_bits_out(data);
		} while (out_count);
	}
}
#endif

/* read "count" bits and return this value */

static uint8_t shift_bits_in(uint8_t count)
{
	uint8_t result;

	DATA_HIZ; /* tristate data */

	result = 0;

	while (count)
	{
		result >>= 1;
		CLK_LOW;
		asm("nop");
		if (DATA_READ)
		{
			result |= 0x80;
			parity++;
		}
		CLK_HIGH;
		count--;
	}

	CLK_LOW;

	return result;
}

/* grand unification that achieves DAP_Transfer, DAP_TransferBlock, and DAP_WriteABORT */

static void dap_transfer(const uint8_t *input, uint8_t *output)
{
	uint8_t transfer_count, transfer_request, swd_request, ack, retry_count;
	uint8_t *response_count, *word_value;
	static uint8_t mask_value[4], match_value[4];

	response_count = output;
	(*response_count) = 0;

	if (0 == (flags & FLAG_WRITEABORT))
	{
		transfer_count = *input;
		/* DAP_TransferBlock has a 16-bit "Transfer Count", where as DAP_Transfer uses 8-bits */
		input += (flags & FLAG_TRANSFERBLOCK) ? 2 : 1;

		/* skip over "Transfer Count" (response_count) and "Transfer Response" fields */
		output += (flags & FLAG_TRANSFERBLOCK) ? 3 : 2;
	}
	else
	{
		transfer_count = 1;
		transfer_request = 0x00;
		swd_request = 0x81;
	}

	while (transfer_count)
	{
		/* reset retry count for this go-around */
		retry_count = 0;

		if (0 == (flags & FLAG_OMITREQUESTDECODE))
		{
			/*
			"transfer_request" is a direct copy of the "Transfer Request" byte provided by the host
			"swd_request" is a derivation of the lower nibble of request send to the ARM processor
			we must maintain two copies of very similar data, as "transfer_request" has extra bit fields
			*/
			transfer_request = *input++;

			/* 
			this can be omitted in a pinch; 
			prevents flags exclusive to DAP_Transfer from appearing in DAP_TransferBlock
			*/
			if (flags & FLAG_TRANSFERBLOCK)
				transfer_request &= 0x0F;

			/* determine if this transfer has a "Transfer Data" field */
			if (0x12 == (transfer_request & 0x32))
				word_value = match_value; /* READ  operation is providing match value */
			else if (0x20 == (transfer_request & 0x32))
				word_value = mask_value;  /* WRITE operation is providing match mask  */
			else
				word_value = NULL;

			if (word_value)
			{
				/* "Transfer Data" field does exist, so store provided value */
				word_value[0] = *input++;
				word_value[1] = *input++;
				word_value[2] = *input++;
				word_value[3] = *input++;
				goto finish_transfer;
			}

			/* compute parity for "swd_request" */
			parity = 0;
			if (transfer_request & 0x01)
				parity++;
			if (transfer_request & 0x02)
				parity++;
			if (transfer_request & 0x04)
				parity++;
			if (transfer_request & 0x08)
				parity++;

			/* reads from AP are pipelined, so we must signal this to the ensuing code */
			if (0x03 == (transfer_request & 0x03))
				flags |= FLAG_PIPELINEDAPREAD;

			/* form 8-bit SWD request ("swd_request") from "transfer_request" */
			swd_request = transfer_request & 0x0F;
			swd_request <<= 1;
			swd_request |= (parity & 0x01) ? 0xA1 : 0x81;

			/*
			in a DAP_TransferBlock, one transfer_request/swd_request applies to all transfers
			so, for a DAP_TransferBlock, we set a flag to skip the above decoding on subsequent iterations
			*/
			if (flags & FLAG_TRANSFERBLOCK)
				flags |= FLAG_OMITREQUESTDECODE;
		}

start_of_request:
		/* shift out 8-bits of SWD request */
		out_count = 8;
		shift_bits_out(swd_request);

		/* one cycle turnaround plus three cycles of ACK */
		ack = shift_bits_in(4);
		ack &= 0xE0;
		ack >>= 5;

		if (2 /* WAIT */ == ack)
		{
			retry_count++;
			shift_bits_in(1); /* turnaround cycle (Figure 2-4 DDI 0316D) */
			if (retry_count < 8)
				goto start_of_request;
			else
				goto finish_transfer;
		}
		
		if (1 /* OK */ != ack)
		{
			/* the ACK was unrecognized / FAULT, so we bail */
			flags |= FLAG_BUSFAULT;
			ack = 4 /* FAULT */;
			goto finish_transfer;
		}

		if (0 == (transfer_request & 0x22))
		{
			/* write */
			shift_bits_in(1); /* turnaround cycle */
			parity = 0;
			out_count = 33;
			shift_bits_out(*input++);
			shift_bits_out(*input++);
			shift_bits_out(*input++);
			shift_bits_out(*input++);
			shift_bits_out(parity);
		}
		else
		{
			/* read */
			parity = 0;
			output[0] = shift_bits_in(8);
			output[1] = shift_bits_in(8);
			output[2] = shift_bits_in(8);
			output[3] = shift_bits_in(8);
			shift_bits_in(1); /* parity */
			if (parity & 0x01)
				ack = 0x08; /* not ORed so as to be consistent with reference implementation */
			shift_bits_in(1); /* end turnaround */

			if (transfer_request & 0x10)
			{
				if (
					( match_value[0] != (output[0] & mask_value[0]) ) ||
					( match_value[1] != (output[1] & mask_value[1]) ) ||
					( match_value[2] != (output[2] & mask_value[2]) ) ||
					( match_value[3] != (output[3] & mask_value[3]) )
				)
					ack |= 0x10;
			}
		}

		/* leave bus in the "IDLE" state, per DDI 0316D */
		out_count = 8;
		shift_bits_out(0x00);

		/* if the transaction is a ReadAP[3] and is the last, morph to a ReadDP[3] */
		if (0x9F == swd_request)
			if (
				( (0 == (flags & FLAG_TRANSFERBLOCK)) && (flags & FLAG_PIPELINEDAPREAD) ) ||
				(  (flags & FLAG_TRANSFERBLOCK) && (1 == transfer_count) && (flags & FLAG_PIPELINEDAPREAD) ) || 
				(  (flags & FLAG_TRANSFERBLOCK) && (2 == transfer_count) && (0 == (flags & FLAG_PIPELINEDAPREAD)) )
			)
				swd_request = 0xBD;

		if (flags & FLAG_PIPELINEDAPREAD)
		{
			flags &= ~FLAG_PIPELINEDAPREAD;
			goto start_of_request;
		}

finish_transfer:
		(*response_count)++;
		transfer_count--;

		if (flags & FLAG_WRITEABORT)
			break;

		if (flags & FLAG_TRANSFERBLOCK)
			*(response_count + 2) = ack;
		else
			*(response_count + 1) = ack;

		if (flags & FLAG_BUSFAULT)
                {
			shift_bits_in(100);
			break;
		}

		if (1 /* OK */ != ack) /* anything but OK is cause to abandon any subsequent entries */
			break;

		if (transfer_request & 0x02)
			output += 4;
	}
}

static void swj_pins(const uint8_t *input, uint8_t *output)
{
	/*
	The CMSIS-DAP specification and Keil reference code is ambiguous and reckless about enabling output drivers.
	An implementation conformant to the standard allows pins to be driven in ways that would damage the hardware.
	This implementation is more cautious by NOT implementing the outputs, except RESET.
	*/

	output[0] = 0x00;

	/* SWCLK in */
	if (CLK_READ)
		output[0] |= 0x01;

	/* SWDAT in */
	if (DATA_READ)
		output[0] |= 0x02;

	/* RESET in */
	if (RESET_READ)
		output[0] |= 0x80;

	if (input[1] & 0x80)
	{
		RESET_ENABLE;
		if (input[0] & 0x80)
		{
			RESET_HIGH;
		}
		else
		{
			RESET_LOW;
		}
	}
}

void dap_handler(uint8_t *RxDataBuffer)
{
	static uint8_t scratchpad[DAP_PACKET_SIZE + 4]; /* buffer is oversized by 4 to allow for simpler code in dap_transfer() */

	/* pre-fill the response with an echo back of the command */
	scratchpad[0] = RxDataBuffer[0];
	scratchpad[1] = 0x00;
	scratchpad[2] = 0x00;

	switch (RxDataBuffer[0])
	{
	case 0x00: /* DAP_Info */
		switch (RxDataBuffer[1])
		{
		case 0xF0: /* Capabilities */
			scratchpad[1] = 0x01; /* len of byte */
			scratchpad[2] = 0x01; /* Capabilities: SWD only */
			break;
		case 0xFE: /* Packet Count */
			scratchpad[1] = 0x01; /* len of byte */
			scratchpad[2] = DAP_PACKET_COUNT;
			break;
		case 0xFF: /* Packet Size */
			scratchpad[1] = 0x02; /* len of short */
			scratchpad[2] = DAP_PACKET_SIZE >> 0;
			scratchpad[3] = DAP_PACKET_SIZE >> 8;
			break;
		}
		break;
	case 0x02: /* DAP_Connect */
		scratchpad[1] = 0x01;
		SWDIO_INIT;
		DATA_ENABLE;
		CLK_ENABLE;
		CLK_LOW;
		RESET_HIZ;
		break;
	case 0x03: /* DAP_Disconnect */
		DATA_HIZ;
		CLK_HIZ;
		RESET_HIZ;
		break;
	case 0x05: /* DAP_Transfer */
		flags = 0x00;
		dap_transfer(RxDataBuffer + 2, scratchpad + 1);
		break;
	case 0x06: /* DAP_TransferBlock */
		flags = FLAG_TRANSFERBLOCK;
		dap_transfer(RxDataBuffer + 2, scratchpad + 1);
		break;
	case 0x08: /* DAP_WriteABORT */
		flags = FLAG_WRITEABORT | FLAG_OMITREQUESTDECODE;
		dap_transfer(RxDataBuffer + 2, scratchpad + 1);
		break;
	case 0x10: /* DAP_SWJ_Pins */
		swj_pins(RxDataBuffer + 1, scratchpad + 1);
		break;
	case 0x12: /* DAP_SWJ_Sequence */
		swj_sequence(RxDataBuffer + 1);
		break;
	case 0x14: /* DAP_JTAG_Sequence */
#ifdef DAP_SUPPORT_JTAG_SEQUENCE
		jtag_sequence(RxDataBuffer + 1);
		break;
#endif
	case 0x15: /* DAP_JTAG_Configure */
	case 0x16: /* DAP_JTAG_IDCODE */
		scratchpad[1] = 0xFF; /* DAP_ERROR */
		break;
	}

	memcpy(RxDataBuffer, scratchpad, DAP_PACKET_SIZE);
}
