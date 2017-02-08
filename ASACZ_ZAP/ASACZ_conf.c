/*
 * ASACZ_conf.c
 *
 *  Created on: Jan 31, 2017
 *      Author: michele
 */


#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "ts_util.h"
#include "timeout_utils.h"
#include "server_thread.h"
#include "simple_server.h"
#include "dbgPrint.h"
#include "ASACSOCKET_check.h"
#include <ASAC_ZigBee_network_commands.h>
#include "input_cluster_table.h"
#include "ZigBee_messages.h"
#include "ASACZ_devices_list.h"
#include "ASACZ_firmware_version.h"
#include "ASACZ_app.h"
#include "ASACZ_conf.h"

#ifdef def_ASACZ_conf_module_test
	#include <ctype.h>
#endif

#define default_channels_mask ( \
							 (1 << 11)	\
							|(1 << 12)	\
							|(1 << 13)	\
							|(1 << 14)	\
							|(1 << 15)	\
							|(1 << 16)	\
							|(1 << 17)	\
							|(1 << 18)	\
							|(1 << 19)	\
							|(1 << 20)	\
							|(1 << 21)	\
							|(1 << 22)	\
							|(1 << 23)	\
							|(1 << 24)	\
							|(1 << 25)	\
							|(1 << 26)	\
						  )

#define default_PAN_id 0xffff

typedef enum
{
	enum_print_int_as_decimal = 0,
	enum_print_int_as_hex,
}enum_print_int;

typedef struct _type_ASACZ_conf_param_table
{
	const char * key;
	enum_ASACZ_conf_param id;
	enum_ASACZ_conf_param_type t;
	type_ASACZ_conf_parameter_value value;
	enum_print_int pi;
}type_ASACZ_conf_param_table;


static type_ASACZ_conf_param_table ASACZ_conf_param[] =
{

#ifdef def_ASACZ_conf_module_test
	{	  .key = "u32"
		, .id = enum_ASACZ_conf_param_u32
		, .t = enum_ASACZ_conf_param_type_uint32
		, .value = {.u32 = 12345678}
	},
	{	  .key = "i32"
		, .id = enum_ASACZ_conf_param_i32
		, .t = enum_ASACZ_conf_param_type_int32
		, .value = {.i32 = -3}
	},
	{	  .key = "u64"
		, .id = enum_ASACZ_conf_param_u64
		, .t = enum_ASACZ_conf_param_type_uint64
		, .value = {.u64 = 0xabbacddeabba}
	},
	{	  .key = "i64"
		, .id = enum_ASACZ_conf_param_i64
		, .t = enum_ASACZ_conf_param_type_int64
		, .value = {.i64 = 0xabbacddeabba}
	},
	{	  .key = "string"
		, .id = enum_ASACZ_conf_param_string
		, .t = enum_ASACZ_conf_param_type_string
		, .value = {.string = {.s = "hello_world"}}
	},
	{	  .key = "byte_array"
		, .id = enum_ASACZ_conf_param_bytearray
		, .t = enum_ASACZ_conf_param_type_byte_array
		, .value = {.byte_array = {.nbytes = 3, .bytes = {1,2,3}}}
	},
#else
	{	  .key = "channels_mask"
		, .id = enum_ASACZ_conf_param_channels_mask
		, .t = enum_ASACZ_conf_param_type_uint32
		, .value = {.u32 = default_channels_mask}
		, .pi =enum_print_int_as_hex
	},
	{	  .key = "PAN_id"
		, .id = enum_ASACZ_conf_param_PAN_id
		, .t = enum_ASACZ_conf_param_type_uint32
		, .value = {.u32 = default_PAN_id}
		, .pi =enum_print_int_as_hex
	},
	{	  .key = "restart_from_scratch"
		, .id = enum_ASACZ_conf_param_restart_network_from_scratch
		, .t = enum_ASACZ_conf_param_type_uint32
		, .value = {.u32 = 0}
		, .pi =enum_print_int_as_decimal
	},
#endif
};
type_ASACZ_conf_param_table * find_key(char * key)
{
	type_ASACZ_conf_param_table *p_entry = NULL;
	unsigned int i;
	for (i = 0; !p_entry && i < sizeof(ASACZ_conf_param) / sizeof(ASACZ_conf_param[0]); i++)
	{
		type_ASACZ_conf_param_table * p = &ASACZ_conf_param[i];
		if (strcasecmp(key, p->key) == 0)
		{
			p_entry = p;
		}
	}
	return p_entry;
}
type_ASACZ_conf_param_table * find_id(enum_ASACZ_conf_param id)
{
	type_ASACZ_conf_param_table *p_entry = NULL;
	unsigned int i;
	for (i = 0; !p_entry && i < sizeof(ASACZ_conf_param) / sizeof(ASACZ_conf_param[0]); i++)
	{
		type_ASACZ_conf_param_table * p = &ASACZ_conf_param[i];
		if (p->id == id)
		{
			p_entry = p;
		}
	}
	return p_entry;
}

#define def_ASACZ_conf_filename "/etc/ASACZ.conf"
static const char *ASACZ_delimiter = "=";
#define def_max_char_line_ASACZ_conf 256


enum_ASACZ_conf_param_type ASACZ_get_conf_param_type(enum_ASACZ_conf_param id)
{
	type_ASACZ_conf_param_table * p = find_id(id);
	if (!p)
	{
		return enum_ASACZ_conf_param_type_invalid;
	}
	return p->t;
}

unsigned int is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param id, type_ASACZ_conf_param *dst)
{
	type_ASACZ_conf_param_table * p = find_id(id);
	if (!p)
	{
		return 0;
	}
	dst->id = id;
	dst->t = p->t;
	dst->value = p->value;
	return 1;
}

uint32_t ASACZ_get_conf_param_channel_mask(void)
{
	type_ASACZ_conf_param dst;
	memset(&dst, 0, sizeof(dst));
	if (is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param_channels_mask, &dst))
	{
		return dst.value.u32;
	}
	return default_channels_mask;
}

uint32_t ASACZ_get_conf_param_PAN_id(void)
{
	type_ASACZ_conf_param dst;
	memset(&dst, 0, sizeof(dst));
	if (is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param_PAN_id, &dst))
	{
		return dst.value.u32;
	}
	return default_PAN_id;
}

uint32_t ASACZ_get_conf_param_restart_from_scratch(void)
{
	type_ASACZ_conf_param dst;
	memset(&dst, 0, sizeof(dst));
	if (is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param_restart_network_from_scratch, &dst))
	{
		return dst.value.u32;
	}
	return default_PAN_id;
}

unsigned int is_OK_ASACZ_set_conf_param(enum_ASACZ_conf_param id, const type_ASACZ_conf_param *src)
{
	type_ASACZ_conf_param_table * p = find_id(id);
	if (!p)
	{
		return 0;
	}
	p->t = src->t;
	p->value = src->value;
	return 1;
}

void ASACZ_reset_conf_param_restart_from_scratch(void)
{
	type_ASACZ_conf_param dst;
	memset(&dst, 0, sizeof(dst));
	if (is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param_restart_network_from_scratch, &dst))
	{
		if (dst.value.u32 > 0)
		{
			dst.value.u32 = 0;
			is_OK_ASACZ_set_conf_param(enum_ASACZ_conf_param_restart_network_from_scratch, &dst);
			ASACZ_save_conf();
		}

	}
}


enum_load_conf_retcode ASACZ_load_conf(void)
{
	enum_load_conf_retcode r = enum_load_conf_retcode_OK;
	FILE *file = NULL;
	uint32_t num_params_loaded = 0;
	if (r == enum_load_conf_retcode_OK)
	{
		file = fopen ( def_ASACZ_conf_filename, "rb" );
		if ( !file )
		{
			my_log(LOG_ERR,"%s: unable to open configuration file %s", __func__, def_ASACZ_conf_filename);
			r = enum_load_conf_retcode_unable_to_open_conf_file;
		}
	}
	if (r == enum_load_conf_retcode_OK)
	{
		char line [ def_max_char_line_ASACZ_conf ]; /* or other suitable maximum line size */
		while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
		{
			char * ptr_key   = strtok(line, ASACZ_delimiter);
			char * ptr_value = strtok(NULL, ASACZ_delimiter);
			if (!ptr_key || !ptr_value)
			{
				my_log(LOG_ERR,"%s: invalid line <%s>", __func__, line);
				if (r == enum_load_conf_retcode_OK)
				{
					r = enum_load_conf_retcode_invalid_line;
				}
				continue;
			}
			type_ASACZ_conf_param_table *p_entry = find_key(ptr_key);
			if (!p_entry)
			{
				my_log(LOG_ERR,"%s: invalid key <%s>", __func__, ptr_key);
				if (r == enum_load_conf_retcode_OK)
				{
					r = enum_load_conf_retcode_invalid_key;
				}
				continue;
			}
			num_params_loaded++;
			switch(p_entry->t)
			{
				case enum_ASACZ_conf_param_type_uint32:
				{
					p_entry->value.u32 = strtoul (ptr_value, NULL, 0);
					break;
				}
				case enum_ASACZ_conf_param_type_int32:
				{
					p_entry->value.i32 = strtol(ptr_value, NULL, 0);
					break;
				}
				case enum_ASACZ_conf_param_type_uint64:
				{
					p_entry->value.u64 = strtoull(ptr_value, NULL, 0);
					break;
				}
				case enum_ASACZ_conf_param_type_int64:
				{
					p_entry->value.i64 = strtoll(ptr_value, NULL, 0);
					break;
				}
				case enum_ASACZ_conf_param_type_string:
				{
					snprintf(p_entry->value.string.s, sizeof(p_entry->value.string.s), "%s", ptr_value);
					break;
				}
				case enum_ASACZ_conf_param_type_byte_array:
				{
					// syntax: 0x12,0x11,27,133
					char *p_next = ptr_value;
					unsigned int idx_byte = 0;
					while (p_next && idx_byte < sizeof(p_entry->value.byte_array.bytes) / sizeof(p_entry->value.byte_array.bytes[0]))
					{
						char * p_next2;
						p_entry->value.byte_array.bytes[idx_byte]= strtoul (p_next, &p_next2, 0);
						idx_byte++;
						if (p_next2)
						{
							p_next = p_next2;
							if (*p_next != ',')
							{
								break;
							}
							p_next ++;
						}
						else
						{
							break;
						}
					}
					p_entry->value.byte_array.nbytes = idx_byte;
					break;
				}
				default:
				{
					my_log(LOG_ERR,"%s: invalid parameter type %u", __func__, (unsigned int)p_entry->t);
					if (r == enum_load_conf_retcode_OK)
					{
						r = enum_load_conf_retcode_invalid_type;
					}
					break;
				}
			}
		}
	}
	if (file)
	{
		if (fclose(file))
		{
			my_log(LOG_ERR,"%s: unable to close configuration file", __func__);
			if (r == enum_load_conf_retcode_OK)
			{
				r = enum_load_conf_retcode_unable_to_close;
			}
		}
	}
	if (!num_params_loaded)
	{
		if (r == enum_load_conf_retcode_OK)
		{
			r = enum_load_conf_retcode_empty_conf_file;
		}
	}
	return r;
}


enum_save_conf_retcode ASACZ_save_conf(void)
{
	enum_save_conf_retcode r = enum_save_conf_retcode_OK;
	FILE *file = NULL;
	uint32_t num_params_saved = 0;

	if (r == enum_save_conf_retcode_OK)
	{
//#error solve the problem related to the permission to write to this file!
		file = fopen ( def_ASACZ_conf_filename, "wb" );
		if ( !file )
		{
			my_log(LOG_ERR,"%s: unable to open write configuration file %s", __func__, def_ASACZ_conf_filename);
			r = enum_save_conf_retcode_unable_to_open_conf_file;
		}
	}
	if (r == enum_save_conf_retcode_OK)
	{
		type_ASACZ_conf_param_table *p_entry = NULL;
		unsigned int i;
		for (i = 0; !p_entry && i < sizeof(ASACZ_conf_param) / sizeof(ASACZ_conf_param[0]); i++)
		{
			type_ASACZ_conf_param_table * p_entry = &ASACZ_conf_param[i];
			char line [ def_max_char_line_ASACZ_conf ];
			char *pc = line;
			int nchar_residual = sizeof(line);
			int nchar = snprintf(pc, nchar_residual, "%s%c", p_entry->key, ASACZ_delimiter[0]);
			if (nchar < 0)
			{
				my_log(LOG_ERR,"%s: unable to sprint key %s", __func__, p_entry->key);
				continue;
			}
			nchar_residual -= nchar;
			if (nchar_residual < 0)
			{
				my_log(LOG_ERR,"%s: no room left after key %s", __func__,p_entry->key);
				continue;
			}
			pc += nchar;
			switch(p_entry->t)
			{
				case enum_ASACZ_conf_param_type_uint32:
				{
					if (p_entry->pi == enum_print_int_as_hex)
					{
						nchar = snprintf(pc, nchar_residual, "0x%"PRIX32, p_entry->value.u32);
					}
					else
					{
						nchar = snprintf(pc, nchar_residual, "%"PRIu32, p_entry->value.u32);
					}
					break;
				}
				case enum_ASACZ_conf_param_type_int32:
				{
					nchar = snprintf(pc, nchar_residual, "%"PRIi32, p_entry->value.i32);
					break;
				}
				case enum_ASACZ_conf_param_type_uint64:
				{
					if (p_entry->pi == enum_print_int_as_hex)
					{
						nchar = snprintf(pc, nchar_residual, "0x%"PRIX64, p_entry->value.u64);
					}
					else
					{
						nchar = snprintf(pc, nchar_residual, "%"PRIu64, p_entry->value.u64);
					}
					break;
				}
				case enum_ASACZ_conf_param_type_int64:
				{
					nchar = snprintf(pc, nchar_residual, "%"PRIi64, p_entry->value.i64);
					break;
				}
				case enum_ASACZ_conf_param_type_string:
				{
					nchar = snprintf(pc, nchar_residual, "%s", p_entry->value.string.s);
					break;
				}
				case enum_ASACZ_conf_param_type_byte_array:
				{
					unsigned int loop_error = 0;
					unsigned int nb;
					for (nb = 0; !loop_error && (nb < p_entry->value.byte_array.nbytes); nb++)
					{
						char c = ',';
						if (nb + 1 >= p_entry->value.byte_array.nbytes)
						{
							c = ' ';
						}
						nchar = snprintf(pc, nchar_residual, "%"PRIu8"%c", p_entry->value.byte_array.bytes[nb], c);
						if (nchar < 0)
						{
							loop_error = 1;
							continue;
						}
						nchar_residual -= nchar;
						if (nchar_residual < 0)
						{
							loop_error = 1;
							continue;
						}
						pc += nchar;
					}
					break;
				}
				default:
				{
					my_log(LOG_ERR,"%s: invalid parameter type %u", __func__, (unsigned int)p_entry->t);
					if (r == enum_save_conf_retcode_OK)
					{
						r = enum_save_conf_retcode_invalid_type;
					}
					break;
				}
			}
			if (nchar < 0)
			{
				my_log(LOG_ERR,"%s: unable to sprint key %s", __func__, p_entry->key);
				continue;
			}
			nchar_residual -= nchar;
			if (nchar_residual < 0)
			{
				my_log(LOG_ERR,"%s: no room left after key %s", __func__, p_entry->key);
				continue;
			}
			if (fprintf(file, "%s\n", line) < 0)
			{
				if (r == enum_save_conf_retcode_OK)
				{
					r = enum_save_conf_retcode_unable_to_write;
				}
			}
			else
			{
				num_params_saved ++;
			}
		}
	}

	if (file)
	{
		if (fclose(file))
		{
			my_log(LOG_ERR,"%s: unable to close configuration file", __func__);
			if (r == enum_save_conf_retcode_OK)
			{
				r = enum_save_conf_retcode_unable_to_close;
			}
		}
	}
	if (!num_params_saved)
	{
		if (r == enum_save_conf_retcode_OK)
		{
			r = enum_save_conf_retcode_empty_conf_file;
		}
	}
	return r;
}

#ifdef def_ASACZ_conf_module_test
static void do_print_conf_param(type_ASACZ_conf_param *dst, unsigned int change)
{
	switch(dst->t)
	{
		case enum_ASACZ_conf_param_type_uint32:
		{
			my_log(LOG_INFO,"u32 %"PRIu32, dst->value.u32);
			if (change) dst->value.u32++;
			break;
		}
		case enum_ASACZ_conf_param_type_int32:
		{
			my_log(LOG_INFO,"i32 %"PRIi32, dst->value.i32);
			if (change) dst->value.i32++;
			break;
		}
		case enum_ASACZ_conf_param_type_uint64:
		{
			my_log(LOG_INFO,"u64 %"PRIu64, dst->value.u64);
			if (change) dst->value.u64++;
			break;
		}
		case enum_ASACZ_conf_param_type_int64:
		{
			my_log(LOG_INFO,"i64 %"PRIi64, dst->value.i64);
			if (change) dst->value.i64++;
			break;
		}
		case enum_ASACZ_conf_param_type_string:
		{
			my_log(LOG_INFO,"s %s", dst->value.string.s);
			int c = dst->value.string.s[0];
			if (isupper(c))
			{
				c = tolower(c);
			}
			else
			{
				c = toupper(c);
			}
			if (change) dst->value.string.s[0] = c;
			break;
		}
		case enum_ASACZ_conf_param_type_byte_array:
		{
			unsigned int nb;
			for (nb = 0; nb < dst->value.byte_array.nbytes; nb++)
			{
				my_log(LOG_INFO,"byte array[%u] %u", nb, (unsigned int )dst->value.byte_array.bytes[nb]);
				if (change) dst->value.byte_array.bytes[nb] ^= 0xff;
			}
			break;
		}
		default:
		{
			my_log(LOG_ERR,"%s: invalid parameter type %u", __func__, (unsigned int)dst->t);
			break;
		}
	}
}
void ASACZ_conf_module_test(void)
{
	enum_load_conf_retcode rload = ASACZ_load_conf();
	if (rload != enum_load_conf_retcode_OK)
	{
		my_log(LOG_ERR,"%s: unable to load the conf", __func__);
	}
	type_ASACZ_conf_param dst;
	enum_ASACZ_conf_param id;
	for (id = enum_ASACZ_conf_param_u32; id < enum_ASACZ_conf_param_numof; id++)
	{
		if (!is_OK_ASACZ_get_conf_param(id, &dst))
		{
			my_log(LOG_ERR,"%s: unable to get parameter %u", __func__, (unsigned int )id);
		}
		do_print_conf_param(&dst, 1);
		if (!is_OK_ASACZ_set_conf_param(id, &dst))
		{
			my_log(LOG_ERR,"%s: unable to set parameter %u", __func__, (unsigned int )id);
		}
		do_print_conf_param(&dst, 0);
	}
	enum_save_conf_retcode rsave = ASACZ_save_conf();
	if (rsave != enum_save_conf_retcode_OK)
	{
		my_log(LOG_ERR,"%s: unable to save the conf", __func__);
	}

	rload = ASACZ_load_conf();
	if (rload != enum_load_conf_retcode_OK)
	{
		my_log(LOG_ERR,"%s: unable to load the conf", __func__);
	}
	for (id = enum_ASACZ_conf_param_u32; id < enum_ASACZ_conf_param_numof; id++)
	{
		if (!is_OK_ASACZ_get_conf_param(id, &dst))
		{
			my_log(LOG_ERR,"%s: unable to get parameter %u", __func__, (unsigned int )id);
		}
		do_print_conf_param(&dst, 0);
	}
}
#endif
