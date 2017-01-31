/*
 * ASACSOCKET_check.c
 *
 *  Created on: Jun 8, 2016
 *      Author: michele
 */


#include "ASACSOCKET_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "crc32.h"

type_struct_ASACSOCKET_check_stats ASACSOCKET_check_stats;

const char * string_of_check_ASACSOCKET_return_code(enum_check_ASACSOCKET_formatted_message e)
{
	const char * p = "ERR_unknown";
	switch(e)
	{
		case enum_check_ASACSOCKET_formatted_message_OK:
		{
			p = "OK";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_key_header:
		{
			p = "ERR_key_header";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_header:
		{
			p = "ERR_length_too_short_no_header";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_body:
		{
			p = "ERR_length_too_short_no_body";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_footer:
		{
			p = "ERR_length_too_short_no_footer";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_length_too_big:
		{
			p = "ERR_length_too_big";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_crc:
		{
			p = "ERR_crc";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_key_footer:
		{
			p = "ERR_key_footer";
			break;
		}
		case enum_check_ASACSOCKET_formatted_message_ERR_unknown:
		default:
		{
			p = "ERR_unknown";
			break;
		}
	};
	return p;
}

// useful to check whether a received message is in the proper format or not
enum_check_ASACSOCKET_formatted_message check_ASACSOCKET_formatted_message(char *buffer, unsigned int buf_len)
{
	enum_check_ASACSOCKET_formatted_message r = enum_check_ASACSOCKET_formatted_message_OK;
	type_struct_ASACSOCKET_msg * p =(type_struct_ASACSOCKET_msg *) buffer;
	type_struct_ASACSOCKET_header * ph = &(p->h);

	// checks if the length is too small
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		if (buf_len < sizeof(*ph))
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_header;
		}
	}
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		if (buf_len < sizeof(*ph) + ph->body_length)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_body;
		}
	}
#ifdef def_enable_ASACSOCKET_footer
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		if (buf_len < sizeof(*ph) + ph->body_length + sizeof(p->f))
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_footer;
		}
	}
#endif
	// checks for the correct header
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		if (ph->key != def_ASACSOCKET_key_header)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_key_header;
		}
	}
	// checks for a good length
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		if (ph->body_length > def_max_length_body_ASACSOCKET_msg)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_length_too_big;
		}
	}
	// checks for a good CRC code
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		uint32_t body_crc_calculated = 0;
		body_crc_calculated = crc32(body_crc_calculated, (const void *)p->body, ph->body_length);
		if (body_crc_calculated != ph->body_crc)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_crc;
		}
	}
#ifdef def_enable_ASACSOCKET_footer
	if (r == enum_check_ASACSOCKET_formatted_message_OK)
	{
		type_struct_ASACSOCKET_footer * pf = (type_struct_ASACSOCKET_footer *)&(p->body[ph->body_length]);
		if (pf->key != def_ASACSOCKET_key_footer)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_key_footer;
		}
	}
#endif
	// update statistics
	{
		if (r >= enum_check_ASACSOCKET_formatted_message_numof)
		{
			r = enum_check_ASACSOCKET_formatted_message_ERR_unknown;
		}
		ASACSOCKET_check_stats.check_retcodes[r]++;
		ASACSOCKET_check_stats.num_check_done++;
	}
	return r;
}


// useful to build a message in the proper format to be sent through ASACSOCKET message
enum_build_ASACSOCKET_formatted_message build_ASACSOCKET_formatted_message(type_struct_ASACSOCKET_msg * p, char *buffer, int buf_len, unsigned int *pui_message_size)
{
	enum_build_ASACSOCKET_formatted_message r = enum_build_ASACSOCKET_formatted_message_OK;
	type_struct_ASACSOCKET_header * ph = &(p->h);

	if (r == enum_build_ASACSOCKET_formatted_message_OK)
	{
		if (buf_len > def_max_length_body_ASACSOCKET_msg)
		{
			r = enum_build_ASACSOCKET_formatted_message_ERR_too_long_body;
		}
	}
	if (r == enum_build_ASACSOCKET_formatted_message_OK)
	{
		memcpy(p->body, buffer, buf_len);
		ph->body_length = buf_len;
		ph->body_crc = crc32(0, (const void *)p->body, ph->body_length);
		ph->key = def_ASACSOCKET_key_header;
#ifdef def_enable_ASACSOCKET_footer
		type_struct_ASACSOCKET_footer * pf = (type_struct_ASACSOCKET_footer *)&(p->body[ph->body_length]);
		pf->key = def_ASACSOCKET_key_footer;
#endif
	}
	if (r == enum_build_ASACSOCKET_formatted_message_OK)
	{
		unsigned int ui_message_size;
		ui_message_size = sizeof(*ph) + ph->body_length;
#ifdef def_enable_ASACSOCKET_footer
		ui_message_size += sizeof(p->f);
#endif
		*pui_message_size = ui_message_size;
	}
	// update statistics
	{
		if (r >= enum_build_ASACSOCKET_formatted_message_numof)
		{
			r = enum_build_ASACSOCKET_formatted_message_ERR_unknown;
		}
		ASACSOCKET_check_stats.build_retcodes[r]++;
		ASACSOCKET_check_stats.num_build_done++;
	}
	return r;
}
