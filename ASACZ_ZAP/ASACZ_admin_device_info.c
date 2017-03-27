/*
 * AZACZ_admin_device_info.c
 *
 *  Created on: Mar 14, 2017
 *      Author: michele
 */

#include <ASACZ_admin_device_info.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <CC2650_fw_update.h>

/*
 * root@OpenWrt:/# touch /etc/config/asaczdevice
root@OpenWrt:/# uci set asaczdevice.serialnumber=123456
root@OpenWrt:/# ls /etc/config/
asaczdevice  dropbear     fstab        network      system       uhttpd
dhcp         firewall     luci         rpcd         ucitrack     wireless
root@OpenWrt:/#
root@OpenWrt:/# ls /etc/config/
root@OpenWrt:/# touch /etc/config/asaczdevice
root@OpenWrt:/# uci set asaczdevice.serialnumber=123456
root@OpenWrt:/# uci show asaczdevice
asaczdevice.serialnumber=123456
root@OpenWrt:/# touch
BusyBox v1.23.2 (2017-03-09 19:30:51 CET) multi-call binary.

Usage: touch [-c] [-d DATE] [-t DATE] [-r FILE] FILE...

Update the last-modified date on the given FILE[s]

-cDon't create files
-d DTDate/time to use
-t DTDate/time to use
-r FILEUse FILE's date/time

root@OpenWrt:/#
root@OpenWrt:/# uci show asaczdevice.serialnumber
123456
root@OpenWrt:/#
root@OpenWrt:/# uci get asaczdevice.serialnumber0
uci: Entry not found
root@OpenWrt:/# uci get asaczdevice.serialnumber
123456
root@OpenWrt:/#
 */

// this command is used to create the file if it doesn't exist
#define def_touch_ASACZ_admin_config_string "touch /etc/config/asaczdevice"

typedef enum
{
	enum_admin_device_info_cmd_touch = 0,
	enum_admin_device_info_cmd_get_sn,
	enum_admin_device_info_cmd_set_sn,
	enum_admin_device_info_cmd_commit,

	enum_admin_device_info_cmd_numof
}enum_admin_device_info_cmd;
typedef enum
{
	enum_admin_device_info_cmd_retcode_OK = 0,
	enum_admin_device_info_cmd_retcode_ERR,

	enum_admin_device_info_cmd_retcode_numof
}enum_admin_device_info_cmd_retcode;

typedef struct _type_pars_in_admin_device_info_cmd_set_sn
{
	uint32_t sn;
}type_pars_in_admin_device_info_cmd_set_sn;


typedef struct _type_pars_out_admin_device_info_cmd_set_sn
{
	uint32_t is_valid;
	uint32_t sn;
}type_pars_out_admin_device_info_cmd_set_sn;

typedef struct _type_pars_out_admin_device_info_cmd_get_sn
{
	uint32_t is_valid;
	uint32_t sn;
}type_pars_out_admin_device_info_cmd_get_sn;

typedef struct _type_pars_out_admin_device_info_cmd_touch
{
	uint32_t is_valid;
}type_pars_out_admin_device_info_cmd_touch;

typedef struct _type_struct_admin_device_info_cmd
{
	enum_admin_device_info_cmd cmd;
	enum_admin_device_info_cmd_retcode retcode;
	union
	{
		type_pars_in_admin_device_info_cmd_set_sn set_sn;
	}pars_in;
	union
	{
		type_pars_out_admin_device_info_cmd_touch touch;
		type_pars_out_admin_device_info_cmd_set_sn set_sn;
		type_pars_out_admin_device_info_cmd_get_sn get_sn;
	}pars_out;
}type_struct_admin_device_info_cmd;

typedef enum
{
	enum_admin_dev_exec_cmd_retcode_OK = 0,
	enum_admin_dev_exec_cmd_retcode_ERR_unable_to_sprint_cmd,
	enum_admin_dev_exec_cmd_retcode_ERR_unknown_cmd,
	enum_admin_dev_exec_cmd_retcode_ERR_calling_command,
	enum_admin_dev_exec_cmd_retcode_ERR_read_reply,
	enum_admin_dev_exec_cmd_retcode_ERR_too_long_reply,

	enum_admin_dev_exec_cmd_retcode_ERR_cmd_exit,
	enum_admin_dev_exec_cmd_retcode_ERR_cmd_signaled,
	enum_admin_dev_exec_cmd_retcode_ERR_cmd_stopped,
	enum_admin_dev_exec_cmd_retcode_ERR_cmd_continued,

	enum_admin_dev_exec_cmd_retcode_ERR_ret_value_uint32_overflow,

	enum_admin_dev_exec_cmd_retcode_numof
}enum_admin_dev_exec_cmd_retcode;

typedef struct _type_table_enum_admin_dev_exec_cmd_retcode_str
{
	enum_admin_dev_exec_cmd_retcode r;
	const char * s;
}type_table_enum_admin_dev_exec_cmd_retcode_str;
static type_table_enum_admin_dev_exec_cmd_retcode_str table_enum_admin_dev_exec_cmd_retcode_str[]=
{
	{.r =enum_admin_dev_exec_cmd_retcode_OK, 							"OK"						},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_unable_to_sprint_cmd,  	"ERR_unable_to_sprint_cmd"	},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_unknown_cmd, 				"ERR_unknown_cmd"			},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_calling_command, 			"ERR_calling_command"		},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_read_reply, 				"ERR_read_reply"			},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_too_long_reply, 			"ERR_too_long_reply"		},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_cmd_exit,					"ERR_cmd_exit"				},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_cmd_signaled, 				"ERR_cmd_signaled"			},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_cmd_stopped, 				"ERR_cmd_stopped"			},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_cmd_continued, 			"ERR_cmd_continued"			},
	{.r =enum_admin_dev_exec_cmd_retcode_ERR_ret_value_uint32_overflow, "ERR_ret_value_uint32_overflow"},

};
const char *get_admin_dev_exec_cmd_retcode_str(enum_admin_dev_exec_cmd_retcode r)
{
	const char *p_ret = "unknown error";
	unsigned int i;
	for (i = 0; i < sizeof(table_enum_admin_dev_exec_cmd_retcode_str) / sizeof(table_enum_admin_dev_exec_cmd_retcode_str[0]); i++)
	{
		type_table_enum_admin_dev_exec_cmd_retcode_str * p = &table_enum_admin_dev_exec_cmd_retcode_str[i];
		if (p->r == r)
		{
			p_ret = p->s;
			break;
		}
	}
	return p_ret;
}

static enum_admin_dev_exec_cmd_retcode admin_dev_exec_cmd(type_struct_admin_device_info_cmd *p)
{
	enum_admin_dev_exec_cmd_retcode r = enum_admin_dev_exec_cmd_retcode_OK;

	FILE *fp = NULL;

	char the_command[512];
	memset(the_command, 0, sizeof(the_command));

	char reply[1024];
	memset(reply, 0, sizeof reply);

	// reset the output parameters
	memset(&p->pars_out, 0, sizeof(p->pars_out));

	if (r == enum_admin_dev_exec_cmd_retcode_OK)
	{
		int nbytes_needed = 0;
		switch (p->cmd)
		{
			case enum_admin_device_info_cmd_touch:
			{
				nbytes_needed = snprintf(the_command, sizeof(the_command), "%s", "touch /etc/config/asaczdevice");
				break;
			}
			case enum_admin_device_info_cmd_get_sn:
			{
				nbytes_needed = snprintf(the_command, sizeof(the_command), "%s", "uci get asaczdevice.serialnumber");
				break;
			}
			case enum_admin_device_info_cmd_set_sn:
			{
				nbytes_needed = snprintf(the_command, sizeof(the_command), "uci set asaczdevice.serialnumber=%u", p->pars_in.set_sn.sn);
				break;
			}
			case enum_admin_device_info_cmd_commit:
			{
				nbytes_needed = snprintf(the_command, sizeof(the_command), "%s", "uci commit asaczdevice");
				break;
			}
			default:
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_unknown_cmd;
				break;
			}
		}
		if (r == enum_admin_dev_exec_cmd_retcode_OK)
		{
			if (nbytes_needed > (int)sizeof(the_command))
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_unable_to_sprint_cmd;
			}
		}
	}

	if (r == enum_admin_dev_exec_cmd_retcode_OK)
	{
		  fp = popen(the_command, "rb");
		  if (fp == NULL)
		  {
			  r = enum_admin_dev_exec_cmd_retcode_ERR_calling_command;
		  }
	}
	if (r == enum_admin_dev_exec_cmd_retcode_OK)
	{
		  // Read the output
		  int n_read = fread(reply, 1, sizeof reply, fp);
		  if (n_read < 0 || n_read > (int) sizeof reply)
		  {
			  r = enum_admin_dev_exec_cmd_retcode_ERR_read_reply;
		  }
		  if (!feof(fp))
		  {
			  r = enum_admin_dev_exec_cmd_retcode_ERR_too_long_reply;
		  }
	}
	if (fp)
	{
		int close_retvalue = pclose(fp);
		if (WIFEXITED(close_retvalue))
		{
			int status = WEXITSTATUS(close_retvalue);
			if (status && (r == enum_admin_dev_exec_cmd_retcode_OK))
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_cmd_exit;
			}
		}
		else if (WIFSIGNALED(close_retvalue))
		{
			//status = WTERMSIG(close_retvalue);
			if (r == enum_admin_dev_exec_cmd_retcode_OK)
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_cmd_signaled;
			}
		}
		else if (WIFSTOPPED(close_retvalue))
		{
			//status = WSTOPSIG(close_retvalue);
			if (r == enum_admin_dev_exec_cmd_retcode_OK)
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_cmd_stopped;
			}
		}
		else if (WIFCONTINUED(close_retvalue))
		{
			if (r == enum_admin_dev_exec_cmd_retcode_OK)
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_cmd_continued;
			}
		}
	}

	if (r == enum_admin_dev_exec_cmd_retcode_OK)
	{
		switch (p->cmd)
		{
			case enum_admin_device_info_cmd_get_sn:
			{
				errno = 0;    /* To distinguish success/failure after call */
				char *endptr;
				uint32_t ui = strtoul(reply, &endptr, 0);
				UINT32_MAX;
				/* Check for various possible errors */
				if (errno == ERANGE && (ui == UINT32_MAX))
				{
					r = enum_admin_dev_exec_cmd_retcode_ERR_ret_value_uint32_overflow;
				}
				else
				{
					p->pars_out.get_sn.is_valid = 1;
					p->pars_out.get_sn.sn = ui;
				}
				break;
			}
			case enum_admin_device_info_cmd_touch:
			{
				p->pars_out.touch.is_valid = 1;
				break;
			}
			case enum_admin_device_info_cmd_set_sn:
			{
				p->pars_out.set_sn.is_valid = 1;
				p->pars_out.set_sn.sn = p->pars_in.set_sn.sn;
				break;
			}
			case enum_admin_device_info_cmd_commit:
			{
				break;
			}
			default:
			{
				r = enum_admin_dev_exec_cmd_retcode_ERR_unknown_cmd;
				break;
			}
		}
	}

	p->retcode = r;

	return r;
}


void get_ASACZ_admin_device_sn(type_administrator_device_info_op_get_serial_number_reply *p_get_reply)
{
	memset(p_get_reply, 0, sizeof(*p_get_reply));

	type_struct_admin_device_info_cmd admin_device_info_cmd;
	memset(&admin_device_info_cmd, 0, sizeof(admin_device_info_cmd));
	admin_device_info_cmd.cmd = enum_admin_device_info_cmd_get_sn;
	enum_admin_dev_exec_cmd_retcode r = admin_dev_exec_cmd(&admin_device_info_cmd);

	if (enum_admin_dev_exec_cmd_retcode_OK == r)
	{
		p_get_reply->is_valid = admin_device_info_cmd.pars_out.get_sn.is_valid;
		p_get_reply->serial_number = admin_device_info_cmd.pars_out.get_sn.sn;
		p_get_reply->retcode.enum_retcode = enum_administrator_device_info_get_sn_retcode_OK;
	}
	else
	{
		p_get_reply->retcode.enum_retcode = enum_administrator_device_info_get_sn_retcode_sn_not_set;
	}
	snprintf((char*)p_get_reply->retcode_ascii, sizeof(p_get_reply->retcode_ascii), "%s", get_admin_dev_exec_cmd_retcode_str(r));
}

typedef struct _type_administrator_device_info_set_sn_retcode_str
{
	enum_administrator_device_info_set_sn_retcode r;
	const char * s;
}type_administrator_device_info_set_sn_retcode_str;

const type_administrator_device_info_set_sn_retcode_str administrator_device_info_set_sn_retcode_str[]=
{
	{.r= enum_administrator_device_info_set_sn_retcode_OK, 							.s = "OK"},
	{.r= enum_administrator_device_info_set_sn_retcode_unable_to_write,				.s = "ERR_unable_to_write"},
	{.r= enum_administrator_device_info_set_sn_retcode_invalid_key,					.s = "ERR_invalid_key"},
	{.r= enum_administrator_device_info_set_sn_retcode_unable_to_calc_key,			.s = "ERR_unable_to_calc_key"},
	{.r= enum_administrator_device_info_set_sn_retcode_error_written_value_differs,	.s = "ERR_written_value_differs"},
};

const char * get_administrator_device_info_set_sn_retcode_str(enum_administrator_device_info_set_sn_retcode r)
{
	const char *  p_ret = "unknown error";
	unsigned int i;
	for (i = 0; i < sizeof(administrator_device_info_set_sn_retcode_str) / sizeof(administrator_device_info_set_sn_retcode_str[0]); i++)
	{
		const type_administrator_device_info_set_sn_retcode_str * p = &administrator_device_info_set_sn_retcode_str[i];
		if (p->r == r)
		{
			p_ret = p->s;
			break;
		}
	}

	return p_ret;
}

void set_ASACZ_admin_device_sn(type_administrator_device_info_op_set_serial_number *p_req, type_administrator_device_info_op_set_serial_number_reply *p_set_reply)
{
	memset(p_set_reply, 0, sizeof(*p_set_reply));
	enum_administrator_device_info_set_sn_retcode proc_retcode = enum_administrator_device_info_set_sn_retcode_OK;

	// check the key
	if (proc_retcode == enum_administrator_device_info_set_sn_retcode_OK)
	{
		unsigned char data_crc[64];
		int n_needed_chars = 0;

		if (proc_retcode  == enum_administrator_device_info_set_sn_retcode_OK)
		{
			n_needed_chars = snprintf((char*)data_crc, sizeof(data_crc), "ASACZsn%u", p_req->new_serial_number);
			if (n_needed_chars > (int) sizeof(data_crc))
			{
				p_set_reply->retcode.enum_retcode = enum_administrator_device_info_set_sn_retcode_unable_to_calc_key;
			}
		}
		if (proc_retcode  == enum_administrator_device_info_set_sn_retcode_OK)
		{
			uint32_t key_validate = calcCrcLikeChip(data_crc, n_needed_chars);
			if (p_req->key_validate != key_validate)
			{
				p_set_reply->retcode.enum_retcode = enum_administrator_device_info_set_sn_retcode_invalid_key;
			}
		}
	}
	if (proc_retcode == enum_administrator_device_info_set_sn_retcode_OK)
	{
		type_struct_admin_device_info_cmd admin_device_info_cmd;
		memset(&admin_device_info_cmd, 0, sizeof(admin_device_info_cmd));
		admin_device_info_cmd.cmd = enum_admin_device_info_cmd_set_sn;
		admin_device_info_cmd.pars_in.set_sn.sn = p_req->new_serial_number;

		enum_admin_dev_exec_cmd_retcode r = admin_dev_exec_cmd(&admin_device_info_cmd);

		if (enum_admin_dev_exec_cmd_retcode_OK == r)
		{

			memset(&admin_device_info_cmd, 0, sizeof(admin_device_info_cmd));
			admin_device_info_cmd.cmd = enum_admin_device_info_cmd_commit;
			enum_admin_dev_exec_cmd_retcode r = admin_dev_exec_cmd(&admin_device_info_cmd);
			if (	enum_admin_dev_exec_cmd_retcode_OK == r)
			{
				memset(&admin_device_info_cmd, 0, sizeof(admin_device_info_cmd));
				admin_device_info_cmd.cmd = enum_admin_device_info_cmd_get_sn;
				enum_admin_dev_exec_cmd_retcode r = admin_dev_exec_cmd(&admin_device_info_cmd);
				if (	enum_admin_dev_exec_cmd_retcode_OK == r
					&& admin_device_info_cmd.pars_out.get_sn.is_valid
					&& admin_device_info_cmd.pars_out.get_sn.sn == p_req->new_serial_number
					)
				{
					p_set_reply->is_valid = 1;
					p_set_reply->serial_number = p_req->new_serial_number;
				}
				else
				{
					proc_retcode = enum_administrator_device_info_set_sn_retcode_error_written_value_differs;
				}
			}
			else
			{
				proc_retcode = enum_administrator_device_info_set_sn_retcode_unable_to_write;
			}

		}
		else
		{
			proc_retcode = enum_administrator_device_info_set_sn_retcode_unable_to_write;
		}

	}
	p_set_reply->retcode.enum_retcode = proc_retcode;
	snprintf((char*)p_set_reply->retcode_ascii, sizeof(p_set_reply->retcode_ascii), "%s", get_administrator_device_info_set_sn_retcode_str(p_set_reply->retcode.enum_retcode));
}
