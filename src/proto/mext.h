/**
 * Copyright (c) 2010 William Light <wrl@illest.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>

#include <monome.h>
#include "internal.h"

#define PACKED __attribute__((__packed__))
#define MONOME_T(ptr) ((monome_t *) ptr)
#define MEXT_T(ptr) ((mext_t *) ptr)

/* protocol constants */

typedef enum {
	SS_SYSTEM      = 0,
	SS_LED_GRID    = 1,
	SS_KEY_GRID    = 2,
	SS_DIGITAL_OUT = 3,
	SS_DIGITAL_IN  = 4,
	SS_ENCODER     = 5,
	SS_ANALOG_IN   = 6,
	SS_ANALOG_OUT  = 7,
	SS_TILT        = 8,
	SS_LED_RING    = 9
} mext_subsystem_t;

typedef enum {
	/* outgoing */
	CMD_SYSTEM_QUERY       = 0x0,
	CMD_SYSTEM_GET_ID      = 0x1,
	CMD_SYSTEM_SET_ID      = 0x2,
	CMD_SYSTEM_GET_OFFSETS = 0x3,
	CMD_SYSTEM_SET_OFFSET  = 0x4,
	CMD_SYSTEM_GET_GRIDSZ  = 0x5,
	CMD_SYSTEM_SET_GRIDSZ  = 0x6,
	CMD_SYSTEM_GET_ADDR    = 0x7,
	CMD_SYSTEM_SET_ADDR    = 0x8,
	CMD_SYSTEM_GET_VERSION = 0xF,
	/* incoming */
	CMD_SYSTEM_QUERY_RESPONSE = 0x0,
	CMD_SYSTEM_ID             = 0x1,
	CMD_SYSTEM_GRID_OFFSET    = 0x2,
	CMD_SYSTEM_GRIDSZ         = 0x3,
	CMD_SYSTEM_ADDR           = 0x4,
	CMD_SYSTEM_VERSION        = 0xF,

	/* outgoing */
	CMD_LED_OFF       = 0x0,
	CMD_LED_ON        = 0x1,
	CMD_LED_ALL_OFF   = 0x2,
	CMD_LED_ALL_ON    = 0x3,
	CMD_LED_FRAME     = 0x4,
	CMD_LED_ROW       = 0x5,
	CMD_LED_COLUMN    = 0x6,
	CMD_LED_INTENSITY = 0x7,
	/* incoming */
	CMD_KEY_UP        = 0x0,
	CMD_KEY_DOWN      = 0x1
} mext_cmd_t;

/* message lengths exclude one-byte header */
static size_t outgoing_payload_lengths[16][16] = {
	[0 ... 15][0 ... 15] = 0,

	[SS_SYSTEM] = {
		[CMD_SYSTEM_QUERY]       = 0,
		[CMD_SYSTEM_GET_ID]      = 0,
		[CMD_SYSTEM_SET_ID]      = 32,
		[CMD_SYSTEM_GET_OFFSETS] = 0,
		[CMD_SYSTEM_SET_OFFSET]  = 3,
		[CMD_SYSTEM_GET_GRIDSZ]  = 0,
		[CMD_SYSTEM_SET_GRIDSZ]  = 2,
		[CMD_SYSTEM_GET_ADDR]    = 0,
		[CMD_SYSTEM_SET_ADDR]    = 2,
		[CMD_SYSTEM_GET_VERSION] = 0,
	},

	[SS_LED_GRID] = {
		[CMD_LED_ON]        = 2,
		[CMD_LED_OFF]       = 2,
		[CMD_LED_ALL_ON]    = 0,
		[CMD_LED_ALL_OFF]   = 0,
		[CMD_LED_FRAME]     = 10,
		[CMD_LED_ROW]       = 3,
		[CMD_LED_COLUMN]    = 3,
		[CMD_LED_INTENSITY] = 1
	}
};

static size_t incoming_payload_lengths[16][16] = {
	[0 ... 15][0 ... 15] = 0,

	[SS_SYSTEM] = {
		[CMD_SYSTEM_QUERY_RESPONSE] = 2,
		[CMD_SYSTEM_ID]             = 32,
		[CMD_SYSTEM_GRID_OFFSET]    = 3,
		[CMD_SYSTEM_GRIDSZ]         = 2,
		[CMD_SYSTEM_ADDR]           = 2,
		[CMD_SYSTEM_VERSION]        = 8
	},

	[SS_KEY_GRID] = {
		[CMD_KEY_DOWN] = 2,
		[CMD_KEY_UP]   = 2
	}
};

/* types */

typedef struct mext mext_t;
typedef struct mext_msg mext_msg_t;

typedef int (*mext_handler_t)(mext_t *, mext_msg_t *, monome_event_t *);

struct mext {
	monome_t monome;
};

struct mext_msg {
	mext_subsystem_t addr;
	mext_cmd_t cmd;

	uint8_t header;

	union {
		uint8_t intensity;
		uint8_t id[32];

		struct {
			uint8_t x;
			uint8_t y;
		} PACKED gridsz;

		struct {
			uint8_t x;
			uint8_t y;
		} PACKED led;

		struct {
			uint8_t x;
			uint8_t y;
			uint8_t data;
		} PACKED led_row_col;

		struct {
			struct {
				uint8_t x;
				uint8_t y;
			} PACKED offset;

			uint8_t data[8];
		} PACKED frame;

		struct {
			uint8_t x;
			uint8_t y;
		} PACKED key;
	} PACKED payload;
} PACKED;