/*
 * ASACZ_conf.h
 *
 *  Created on: Jan 31, 2017
 *      Author: michele
 */

#ifndef ASACZ_CONF_H_
#define ASACZ_CONF_H_
//#define def_ASACZ_conf_module_test

typedef enum
{
#ifdef def_ASACZ_conf_module_test
	enum_ASACZ_conf_param_u32 = 0,
	enum_ASACZ_conf_param_i32,
	enum_ASACZ_conf_param_u64,
	enum_ASACZ_conf_param_i64,
	enum_ASACZ_conf_param_string,
	enum_ASACZ_conf_param_bytearray,
#else
	enum_ASACZ_conf_param_channels_mask = 0,
	enum_ASACZ_conf_param_PAN_id,
#endif
	enum_ASACZ_conf_param_numof
}enum_ASACZ_conf_param;

typedef enum
{
	enum_ASACZ_conf_param_type_invalid = 0,
	enum_ASACZ_conf_param_type_uint32,
	enum_ASACZ_conf_param_type_int32,
	enum_ASACZ_conf_param_type_uint64,
	enum_ASACZ_conf_param_type_int64,
	enum_ASACZ_conf_param_type_string,
	enum_ASACZ_conf_param_type_byte_array,

	enum_ASACZ_conf_param_type_numof
}enum_ASACZ_conf_param_type;

#define def_max_char_string_byte_array 64

typedef struct _type_ASACZ_conf_byte_array
{
	uint32_t nbytes;
	uint8_t bytes[def_max_char_string_byte_array];
}type_ASACZ_conf_byte_array;

typedef struct _type_ASACZ_conf_string
{
	char s[def_max_char_string_byte_array + 1];
}type_ASACZ_conf_string;

typedef union
{
	uint32_t u32;
	int32_t i32;
	uint64_t u64;
	int64_t i64;
	type_ASACZ_conf_string string;
	type_ASACZ_conf_byte_array byte_array;
}type_ASACZ_conf_parameter_value;

typedef enum
{
	enum_load_conf_retcode_OK = 0,
	enum_load_conf_retcode_unable_to_open_conf_file,
	enum_load_conf_retcode_invalid_line,
	enum_load_conf_retcode_invalid_key,
	enum_load_conf_retcode_invalid_type,
	enum_load_conf_retcode_unable_to_close,
	enum_load_conf_retcode_empty_conf_file,

	enum_load_conf_retcode_numof
}enum_load_conf_retcode;

enum_load_conf_retcode ASACZ_load_conf(void);

typedef enum
{
	enum_save_conf_retcode_OK = 0,
	enum_save_conf_retcode_unable_to_open_conf_file,
	enum_save_conf_retcode_invalid_type,
	enum_save_conf_retcode_unable_to_write,
	enum_save_conf_retcode_unable_to_close,
	enum_save_conf_retcode_empty_conf_file,

	enum_save_conf_retcode_numof
}enum_save_conf_retcode;

enum_save_conf_retcode ASACZ_save_conf(void);

enum_ASACZ_conf_param_type ASACZ_get_conf_param_type(enum_ASACZ_conf_param id);

typedef struct _type_ASACZ_conf_param
{
	enum_ASACZ_conf_param id;
	enum_ASACZ_conf_param_type t;
	type_ASACZ_conf_parameter_value value;
}type_ASACZ_conf_param;

unsigned int is_OK_ASACZ_get_conf_param(enum_ASACZ_conf_param id, type_ASACZ_conf_param *dst);
unsigned int is_OK_ASACZ_set_conf_param(enum_ASACZ_conf_param id, const type_ASACZ_conf_param *src);

#ifdef def_ASACZ_conf_module_test
	void ASACZ_conf_module_test(void);
#endif
#endif /* ASACZ_CONF_H_ */
