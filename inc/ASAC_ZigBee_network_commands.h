/*
 * ASAC_ZigBee_network_commands.h
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#ifndef INC_ASAC_ZIGBEE_NETWORK_COMMANDS_H_
#define INC_ASAC_ZIGBEE_NETWORK_COMMANDS_H_

#include <stdint.h>

// PLEASE change this only if a major change in the protocol is done, i.e. if the type_ASAC_Zigbee_interface_header structure changes
#define def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MAJOR_VERSION ((uint32_t) 0 )
// change this if a minor change in the protocol is done, i.e.  commands are added/deleted/modified
#define def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MINOR_VERSION ((uint32_t) 1 )
#define def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_BUILD_NUMBER ((uint32_t) 1 )

// a reserved LinkId: not a valid reply
#define defReservedLinkId_notAReply ((uint32_t)0x00000000)
// the very first valid LinkId
#define defFirstValidLinkId ((uint32_t)0x10000000)
// the very last valid LinkId, the next one to be used is defFirstValidLinkId
#define defLastValidLinkId  ((uint32_t)0x7fffffff)

typedef struct _type_ASAC_Zigbee_interface_protocol_header
{
	uint32_t major_protocol_id; // the major protocol identifier: this must be changed ONLY if the structure of the header following the minor_protocol_id is changed
								// e.g. let's suppose the code field becomes a uint64:t, or we put a field named key BEFORE the code field: then the major_protocol_id must be changed
								// if a message comes with a major_protocol_id different from mine, I can't reply
								// the very first major_protocol_id is 0
	uint32_t minor_protocol_id; // the minor protocol identifier: this can be changed when command are added/deleted/modified
}__attribute__((__packed__)) type_ASAC_Zigbee_interface_protocol_header;

typedef struct _type_ASAC_Zigbee_interface_command_header
{
	uint32_t command_version; 	// the command version requested/replied; when a specific command structure is changed, the command_version must be changed
								// the very first command_version is 0
	uint32_t command_code; 		// the command code, MUST be one of enum_ASAC_ZigBee_interface_command
	uint32_t command_link_id; 	// the link identifier, most of the times this is set by a request and copied back by a reply
								// some values are reserved, e.g. 0x00000000 states the message isn't a reply to a previous request nor it expects a reply
}__attribute__((__packed__)) type_ASAC_Zigbee_interface_command_header;

typedef struct _type_ASAC_Zigbee_interface_reserved_header
{
	uint32_t reserved[4];		// reserved for future implementations
}__attribute__((__packed__)) type_ASAC_Zigbee_interface_reserved_header;

typedef struct _type_ASAC_Zigbee_interface_header
{
	type_ASAC_Zigbee_interface_protocol_header p;
	type_ASAC_Zigbee_interface_command_header c;
	type_ASAC_Zigbee_interface_reserved_header r;
}__attribute__((__packed__)) type_ASAC_Zigbee_interface_header;

typedef enum
{
// the commands we send to/receive from the outside world, e.g. the user messages
	enum_ASAC_ZigBee_interface_command_outside_first = 0x10000000,
// sends a user message
	enum_ASAC_ZigBee_interface_command_outside_send_message = enum_ASAC_ZigBee_interface_command_outside_first,
// a user message has been received
	enum_ASAC_ZigBee_interface_command_outside_received_message,

// the commands we use to handle the network, e.g. the introduce myself which sets my name in the network
	enum_ASAC_ZigBee_interface_command_network_first = 0x20000000,
// create and announce an input cluster
	enum_ASAC_ZigBee_interface_command_network_input_cluster_register_req = enum_ASAC_ZigBee_interface_command_network_first,
// echo the body
	enum_ASAC_ZigBee_interface_command_network_echo_req,
// unregister an input cluster
	enum_ASAC_ZigBee_interface_command_network_input_cluster_unregister_req,
// device list request
	enum_ASAC_ZigBee_interface_command_network_device_list_req,
// server program firmware version request
	enum_ASAC_ZigBee_interface_command_network_firmware_version_req,
// device list has changed, sent from the server to all the clients with registered commands/cluster
	enum_ASAC_ZigBee_interface_command_device_list_changed_signal,
// my own IEEE address request
	enum_ASAC_ZigBee_interface_command_network_my_IEEE_req,
// signal strength request
	enum_ASAC_ZigBee_interface_command_network_signal_strength_req,
// probe network-specific properties
	enum_ASAC_ZigBee_interface_command_network_probe,

// the commands used by the administrator
	enum_ASAC_ZigBee_interface_command_administrator_first = 0x30000000,
	enum_ASAC_ZigBee_interface_command_administrator_firmware_update = enum_ASAC_ZigBee_interface_command_administrator_first,
	enum_ASAC_ZigBee_interface_command_administrator_restart_network_from_scratch,
	enum_ASAC_ZigBee_interface_command_administrator_diagnostic_test,
	enum_ASAC_ZigBee_interface_command_administrator_device_info,
	enum_ASAC_ZigBee_interface_command_administrator_system_reboot,
	enum_ASAC_ZigBee_interface_command_administrator_UTC,
	enum_ASAC_ZigBee_interface_command_administrator_restart_me,
// the command used to reply to unknown commands
	enum_ASAC_ZigBee_interface_command_unknown = 0xFFFFFFFF,

}enum_ASAC_ZigBee_interface_command;

typedef struct _type_ASAC_ZigBee_dst_id
{
	uint64_t IEEE_destination_address; 	// the IEEE destination address; 0 always means the coordinator is the destination
	uint8_t destination_endpoint;		// the destination end-point
	uint16_t cluster_id;				// the cluster (command) identifier
	uint8_t source_endpoint;			// the source end-point
	uint8_t transaction_id;				// the transaction identifier
}__attribute__((__packed__)) type_ASAC_ZigBee_dst_id ;

typedef struct _type_ASAC_ZigBee_src_id
{
	uint64_t IEEE_source_address; 	// the IEEE source address; 0 always means the coordinator is the source
	uint8_t source_endpoint;		// the source end-point
	uint16_t cluster_id;			// the cluster (command) identifier
	uint8_t destination_endpoint;	// the destination end-point
	uint8_t transaction_id;			// the transaction identifier
}__attribute__((__packed__)) type_ASAC_ZigBee_src_id ;

//
//
//
// input cluster register begins here
//
//
//
#define def_input_cluster_register_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_input_cluster_register_req
{
	uint8_t endpoint;			// the end-point to register
	uint16_t input_cluster_id;	// the input cluster (command) to register
}  __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_input_cluster_register_req;

typedef enum
{
	enum_input_cluster_register_reply_retcode_OK = 0,
	enum_input_cluster_register_reply_retcode_ERR_no_room,
	enum_input_cluster_register_reply_retcode_ERR_invalid_endpoint,
	enum_input_cluster_register_reply_retcode_ERR_unknown,
	enum_input_cluster_register_reply_retcode_numof
}enum_input_cluster_register_reply_retcode;

typedef struct _type_ASAC_ZigBee_interface_network_input_cluster_register_reply
{
	uint8_t endpoint;			// the end-point to register
	uint16_t input_cluster_id;	// the input cluster (command) to register
	uint32_t retcode;	// the return code
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_input_cluster_register_reply ;

#define def_input_cluster_unregister_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_input_cluster_unregister_req
{
	uint8_t endpoint;			// the end-point to register
	uint16_t input_cluster_id;	// the input cluster (command) to register
}  __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_input_cluster_unregister_req;

typedef enum
{
	enum_input_cluster_unregister_reply_retcode_OK = 0,
	enum_input_cluster_unregister_reply_retcode_ERR_not_found,
	enum_input_cluster_unregister_reply_retcode_numof
}enum_input_cluster_unregister_reply_retcode;

typedef struct _type_ASAC_ZigBee_interface_network_input_cluster_unregister_reply
{
	uint8_t endpoint;			// the end-point to register
	uint16_t input_cluster_id;	// the input cluster (command) to register
	enum_input_cluster_unregister_reply_retcode retcode;	// the return code
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_input_cluster_unregister_reply ;

//
//
//
// input cluster register ends here
//
//
//


//
//
//
// echo begins here
//
//
//
#define def_echo_req_command_version 0
#define def_echo_message_max_bytes 32
typedef struct _type_ASAC_ZigBee_interface_network_echo_req
{
	uint8_t message_to_echo[def_echo_message_max_bytes];			// the message to echo
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_echo_req;

typedef struct _type_ASAC_ZigBee_interface_network_echo_reply
{
	uint8_t message_to_echo[def_echo_message_max_bytes];			// the message to echo
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_echo_reply ;

//
//
//
// echo ends here
//
//
//




//
//
//
// device list begins here
//
//
//
#define def_device_list_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_device_list_req
{
	uint32_t start_index; 	//!< 0 means from the very beginning
	uint32_t sequence; 		//!< if index > 0, this should be the same received when the very first message was sent
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_device_list_req;


#define def_max_device_list_chunk 128
typedef struct _type_device_list_chunk
{
	uint64_t	IEEE_address;				//!< the IEEE 64-bit address of the device
}__attribute__((__packed__)) type_device_list_chunk;

typedef struct _type_ASAC_ZigBee_interface_network_device_list_reply
{
	uint32_t start_index;			//!< the index of the first element in list
	uint32_t sequence; 				//!< the current sequence number
	uint32_t sequence_valid;		//!< this is set to 1 if the request is valid; if not, the client must restart the list request from start_index 0
									//!< because the device list has changed
	uint32_t list_ends_here;		//!< this is set to 1 if the device list has been fully reported, if it is 0, another query starting from start_index+num_devices_in_chunk is needed
	uint32_t num_devices_in_chunk;	//!< the number of devices in the chunk
	type_device_list_chunk list_chunk[def_max_device_list_chunk];
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_device_list_reply ;

//
//
//
// device list ends here
//
//
//


//
//
//
// my IEEE begins here
//
//
//
#define def_my_IEEE_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_my_IEEE_req
{
	uint32_t unused; 	//!< not used
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_my_IEEE_req;


typedef struct _type_ASAC_ZigBee_interface_network_my_IEEE_reply
{
	uint32_t is_valid_IEEE_address;			//!< if 0, the IEEE address is not valid
	uint64_t IEEE_address; 					//!< the current sequence number
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_my_IEEE_reply ;

//
//
//
// my IEEE ends here
//
//
//


//
//
//
// firmware version begins here
//
//
//
/**
 * the firmware version request
 */
#define def_firmware_version_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_firmware_version_req
{
	uint32_t as_yet_unused; 	//!< as the name says
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_firmware_version_req;


/**
 * the firmware version reply
 */
typedef struct _type_ASAC_ZigBee_interface_network_firmware_version_reply
{
	uint32_t major_number;			//!< the version major number
	uint32_t middle_number;			//!< the version middle number
	uint32_t minor_number;			//!< the version minor number
	uint32_t build_number;			//!< the build number
	uint8_t	date_and_time[64];		//!< the version date and time
	uint8_t	patch[64];				//!< the version patch
	uint8_t	notes[64];				//!< the version notes
	uint8_t	string[256];			//!< the version string
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_firmware_version_reply ;

//
//
//
// firmware version ends here
//
//
//



//
//
//
// signal strength request begins here
//
//
//
#define def_signal_strength_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_signal_strength_req
{
	uint32_t unused;
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_network_signal_strength_req;

typedef struct _type_ASAC_ZigBee_interface_network_signal_strength_reply
{
	uint32_t level_min0_max4; 	// 4 = excellent, 3 = good, 2 = normal, 1 = low, 0 = unknown
	uint32_t v_0_255;			// the value between 0..255, this is the one value the radio chip sends me
	int32_t v_dBm;				// the value in dBm I extrapolate, usually this is less than zero!
	int64_t milliseconds_ago; 	// states how many milliseconds ago was the latest signal strength received
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_signal_strength_reply ;

//
//
//
// signal strength ends here
//
//
//



//
//
//
// network restart from scratch request begins here
//
//
//
#define def_restart_network_from_scratch_req_command_version 0
typedef struct _type_ASAC_ZigBee_interface_restart_network_from_scratch_req
{
	uint32_t unused;
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_restart_network_from_scratch_req;

typedef struct _type_ASAC_ZigBee_interface_restart_network_from_scratch_reply
{
	uint32_t restart_required_OK; 	// 0 = error, else OK
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_restart_network_from_scratch_reply ;

//
//
//
// signal strength ends here
//
//
//


#define def_device_list_changed_signal_command_version 0
typedef struct _type_ASAC_ZigBee_interface_network_device_list_changed_signal
{
	uint32_t sequence_number;
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_device_list_changed_signal;


//
//
//
// SEND OUTSIDE MESSAGE begins here
//
//
//
#define def_outside_send_message_req_command_version 0
// the maximum size for a message to send to / receive from the outside world
#define def_ASAC_ZigBee_interface_max_size_outside_message 128

typedef struct _type_ASAC_ZigBee_interface_command_outside_send_message_req
{
	type_ASAC_ZigBee_dst_id dst_id;		// the destination identifier
	uint8_t message_length;				// the number of bytes in the message
	uint8_t message[def_ASAC_ZigBee_interface_max_size_outside_message];	// the message to send
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_command_outside_send_message_req ;
/**
 * use this define to get the meaningful number of bytes in the outside message request structure
 */
#define NUM_BYTES_IN_outside_send_message_req(pm) (sizeof((pm)->dst_id) + sizeof((pm)->message_length) + (pm)->message_length)

// the return codes from the send user message outside
typedef enum
{
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK = 0,
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_ERROR_unable_to_push_message, // no room in the radio messages queue
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_numof
}enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode;

typedef struct _type_ASAC_ZigBee_interface_command_outside_send_message_reply
{
	type_ASAC_ZigBee_dst_id dst_id;		// the destination identifier
	uint8_t message_length;				// the number of bytes in the message
	uint32_t retcode; 	// must be of type enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode
	uint32_t id; 		// the message identifier / handle before it leaves the radio
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_command_outside_send_message_reply;

//
//
//
// SEND OUTSIDE MESSAGE ends here
//
//
//


// received message callback begins here
#define def_outside_received_message_callback_command_version 0
typedef struct _type_ASAC_ZigBee_interface_command_received_message_callback
{
	type_ASAC_ZigBee_src_id src_id;		// the source identifier
	uint8_t message_length;				// the number of bytes in the message
	uint8_t message[def_ASAC_ZigBee_interface_max_size_outside_message];	// the message received
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_command_received_message_callback ;


/**
 * use this define to get the meaningful number of bytes in the outside message request structure
 */
#define NUM_BYTES_IN_command_received(pm) (sizeof((pm)->src_id) + sizeof((pm)->message_length) + (pm)->message_length)

typedef struct _type_ASAC_ZigBee_interface_unknown_reply
{
	uint32_t the_unknown_code; // the unknown code we got
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_unknown_reply;


//
// firmware update begins here
//

// the destination of the firmware update
typedef enum
{
	enum_ASAC_ZigBee_fwupd_destination_CC2650 = 0,
	enum_ASAC_ZigBee_fwupd_destination_ASACZ,
	enum_ASAC_ZigBee_fwupd_destination_numof
}enum_ASAC_ZigBee_fwupd_destination;

// the CC2650 radio chip firmware operations available
typedef enum
{
	enum_ASAC_ZigBee_fwupd_CC2650_op_read_version = 0,		// read the radio chip firmware version
	enum_ASAC_ZigBee_fwupd_CC2650_op_start_update,			// update the radio chip firmware
	enum_ASAC_ZigBee_fwupd_CC2650_op_query_update_status,	// query the status of the radio chip firmware update procedure
	enum_ASAC_ZigBee_fwupd_CC2650_op_query_firmware_file,	// query the firmware informations embedded in a CC2650 ASAC firmware file

	enum_ASAC_ZigBee_fwupd_CC2650_numof
}enum_ASAC_ZigBee_fwupd_CC2650_ops;

// the ASACZ firmware operations available
typedef enum
{
	enum_ASAC_fwupd_ASACZ_op_start_update = 0,		// starts the update using the package file already transferred in a local directory
	enum_ASAC_fwupd_ASACZ_op_status_update,			// checks the update status

	enum_ASAC_fwupd_ASACZ_op_numof
}enum_ASAC_fwupd_ASACZ_ops;


typedef struct _type_fwupd_CC2650_read_version_req_body
{
	uint32_t unused;
}__attribute__((__packed__)) type_fwupd_CC2650_read_version_req_body;


typedef struct _type_fwupd_CC2650_read_version_reply_body
{
	uint32_t is_valid;
	uint32_t major;
	uint32_t middle;
	uint32_t minor;
	uint32_t product;
	uint32_t transport;
}__attribute__((__packed__)) type_fwupd_CC2650_read_version_reply_body;


#define def_max_length_fwupd_ASACZ_fw_signed_filename 256
typedef struct _type_fwupd_ASACZ_do_update_req_body
{
	uint8_t ASACZ_fw_signed_filename[def_max_length_fwupd_ASACZ_fw_signed_filename]; // the filename is relative to the default base directory
}__attribute__((__packed__)) type_fwupd_ASACZ_do_update_req_body;

#define def_max_length_fwupd_ASACZ_fw_update_error_message 256
typedef struct _type_fwupd_ASACZ_do_update_reply_body
{
	uint32_t started_OK;	// this is set to 1 if the requested was forward OK
}__attribute__((__packed__)) type_fwupd_ASACZ_do_update_reply_body;

typedef struct _type_fwupd_ASACZ_status_update_req_body
{
	uint32_t unused;
}__attribute__((__packed__)) type_fwupd_ASACZ_status_update_req_body;



typedef enum
{
	enum_file_package_progress_init,
	enum_file_package_progress_input_file_read,
	enum_file_package_progress_old_file_backup_done,
	enum_file_package_progress_new_file_copy_done,
	enum_file_package_progress_old_file_restore_done,
	enum_file_package_progress_ends,
	enum_file_package_progress_numof
}enum_file_package_progress;

typedef struct _type_fwupd_ASACZ_status_update_reply_body
{
	union
	{
		enum_file_package_progress enum_p;
		uint32_t uint;
	}u_progress;
	uint32_t progress_0_100;
	uint32_t running;
	uint32_t ends_OK;			// this is set to 1 if the requested was forward OK
	uint32_t ends_ERROR;			// this is set to 1 if the requested was forward OK
	uint32_t result_code;	// the result code please see enum_request_ASACZ_firmware_update_retcode, anyway 0 means OK
	uint8_t result_message[def_max_length_fwupd_ASACZ_fw_update_error_message];	// the result string message, we hope it will be "OK"
}__attribute__((__packed__)) type_fwupd_ASACZ_status_update_reply_body;

#define def_max_length_fwupd_CC2650_fw_signed_filename 256
typedef struct _type_fwupd_CC2650_start_update_req_body
{
	uint8_t CC2650_fw_signed_filename[def_max_length_fwupd_CC2650_fw_signed_filename];
}__attribute__((__packed__)) type_fwupd_CC2650_start_update_req_body;

#define def_max_length_fwupd_CC2650_fw_update_error_message 256
typedef struct _type_fwupd_CC2650_start_update_reply_body
{
	uint32_t is_OK;			// this is set to 1 if the requested was forward OK
	uint32_t num_request;	// the identifier of the firmware update request assigned
	uint32_t result_code;	// the result code please see enum_request_CC2650_firmware_update_retcode, anyway 0 means OK
	uint8_t result_message[def_max_length_fwupd_CC2650_fw_update_error_message];	// the result string message, we hope it will be "OK"
}__attribute__((__packed__)) type_fwupd_CC2650_start_update_reply_body;

typedef struct _type_fwupd_CC2650_query_update_status_req_body
{
	uint32_t unused;
}__attribute__((__packed__)) type_fwupd_CC2650_query_update_status_req_body;

#define def_max_char_fwupd_CC2650_result_string 256
typedef struct _type_fwupd_CC2650_query_update_status_reply_body
{
	uint32_t status;							// please see enum_CC2650_fw_update_status
	uint32_t ends_OK;							// the update has finished OK
	uint32_t ends_ERR;							// the update has finished with error
	uint32_t fw_update_result_code_is_valid;	// the following field fw_update_result_code is valid
	uint32_t fw_update_result_code;				// the result of the firmware update procedure, please see enum_do_CC2650_fw_update_retcode
	uint8_t fw_update_result_string[def_max_char_fwupd_CC2650_result_string];	// the firmware update result string
	uint32_t flash_write_percentage; 			// the percentage of the CC2650 flash write procedure
	uint32_t num_request;						// this is the current flash update request progressive index; most of the times it will be the same value returned on start update
	uint32_t num_ack;							// this is the current request number being serviced / completed
}__attribute__((__packed__)) type_fwupd_CC2650_query_update_status_reply_body;

typedef struct _type_fwupd_CC2650_query_firmware_file_req
{
	uint8_t CC2650_fw_query_filename[def_max_length_fwupd_CC2650_fw_signed_filename];
}__attribute__((__packed__)) type_fwupd_CC2650_query_firmware_file_req;

typedef struct _type_fwupd_CC2650_query_firmware_file_reply
{
	uint32_t retcode;					// this is 0 if the check was OK, else it contains the error code, please see enum_do_CC2650_fw_update_retcode
	uint8_t query_result_string[def_max_char_fwupd_CC2650_result_string];	// the query result string
	uint8_t CC2650_fw_query_filename[def_max_length_fwupd_CC2650_fw_signed_filename];	// the filename queried
	uint8_t magic_name[32];				// must be set to "ASACZ_CC2650_fw_update_header", and filled up with 0x00 in the remaining bytes
	uint8_t ascii_fw_type[32];			// must be set to "COORDINATOR" or "ROUTER" or "END_DEVICE"
	uint8_t ascii_version_number[32];	// must have the format "<major>.<middle>.<minor>"
	uint8_t date[32];					// must be set to "YYYY mmm dd"
	uint32_t fw_type;					// must be set to 0 for COORDINATOR, 1 for ROUTER, 2 for END_DEVICE
	uint32_t fw_version_major;			// the firmware version major number
	uint32_t fw_version_middle;			// the firmware version middle number
	uint32_t fw_version_minor;			// the firmware version minor number
	uint32_t firmware_body_size;		// the expected number of bytes in the firmware body, most of the times it should be 131072, i.e. 128 kBytes
	uint32_t firmware_body_CRC32_CC2650;// the CRC32 of the firmware body calculated as CC2650 does it, please see the calcCrcLikeChip routine
	uint32_t header_CRC32_CC2650;		// the CRC32 of the header (this field excluded), calculated as CC2650 does it, please see the calcCrcLikeChip routine
}__attribute__((__packed__)) type_fwupd_CC2650_query_firmware_file_reply;


#define def_ASAC_ZigBee_fwupd_req_command_version 0

typedef struct _type_ASAC_ZigBee_interface_command_fwupd_req
{
	// the destination of the firmware update command
	// reading this field I can state the format of the following enumeration field
	union
	{
		enum_ASAC_ZigBee_fwupd_destination enum_dst;
		uint32_t uint32_dst;
	}dst;
	// the specific operation needed on the destination
	// reading this field I can state the format of the following body
	union
	{
		enum_ASAC_ZigBee_fwupd_CC2650_ops CC2650;
		enum_ASAC_fwupd_ASACZ_ops ASACZ;
		uint32_t uint32_op;
	}ops;
	union
	{
		type_fwupd_CC2650_read_version_req_body CC2650_read_firmware_version;
		type_fwupd_CC2650_start_update_req_body CC2650_start_firmware_update;
		type_fwupd_CC2650_query_update_status_req_body CC2650_query_firmware_update_status;
		type_fwupd_CC2650_query_firmware_file_req CC2650_query_firmware_file_req;

		type_fwupd_ASACZ_do_update_req_body ASACZ_do_update_req_body;
		type_fwupd_ASACZ_status_update_req_body ASACZ_status_update_req_body;
	}body;
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_command_fwupd_req ;


typedef struct _type_ASAC_ZigBee_interface_command_fwupd_reply
{
	// the destination of the firmware update command
	// reading this field I can state the format of the following enumeration field
	union
	{
		enum_ASAC_ZigBee_fwupd_destination enum_dst;
		uint32_t uint32_dst;
	}dst;
	// the specific operation needed on the destination
	// reading this field I can state the format of the following body
	union
	{
		enum_ASAC_ZigBee_fwupd_CC2650_ops CC2650;
		enum_ASAC_fwupd_ASACZ_ops ASACZ;
		uint32_t uint32_op;
	}ops;
	union
	{
		type_fwupd_CC2650_read_version_reply_body CC2650_read_firmware_version;
		type_fwupd_CC2650_start_update_reply_body CC2650_start_firmware_update;
		type_fwupd_CC2650_query_update_status_reply_body CC2650_query_firmware_update_status;
		type_fwupd_CC2650_query_firmware_file_reply CC2650_query_firmware_file_reply;

		type_fwupd_ASACZ_do_update_reply_body ASACZ_do_update_reply_body;
		type_fwupd_ASACZ_status_update_reply_body ASACZ_status_update_reply_body;
	}body;
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_command_fwupd_reply;
//
// firmware update ends here
//


//
// enum_ASAC_ZigBee_interface_command_administrator_diagnostic_test begins here
//


// START
typedef enum
{
	enum_admin_diag_test_op_start = 0,
	enum_admin_diag_test_op_read_status,
	enum_admin_diag_test_op_stop,
	enum_admin_diag_test_op_numof
}enum_admin_diag_test_op;

typedef enum
{
	enum_admin_diag_test_message_length_type_default = 0,
	enum_admin_diag_test_message_length_type_very_short,
	enum_admin_diag_test_message_length_type_short,
	enum_admin_diag_test_message_length_type_medium,
	enum_admin_diag_test_message_length_type_large,
	enum_admin_diag_test_message_length_type_very_large,
	enum_admin_diag_test_message_length_type_custom,
	enum_admin_diag_test_message_length_type_numof
}enum_admin_diag_test_message_length_type;

typedef enum
{
	enum_admin_diag_test_message_body_type_default = 0,
	enum_admin_diag_test_message_body_type_ASCII,
	enum_admin_diag_test_message_body_type_random_binary,
	enum_admin_diag_test_message_body_type_numof
}enum_admin_diag_test_message_body_type;

typedef enum
{
	enum_admin_diag_test_average_type_default = 0,
	enum_admin_diag_test_average_type_uniform_weight,
	enum_admin_diag_test_average_type_raised_cosin,
	enum_admin_diag_test_average_type_numof
}enum_admin_diag_test_average_type;

typedef struct _type_admin_diag_test_req_start_body
{
	uint64_t server_IEEE_address; // 0 means use the coordinator
	union
	{
		enum_admin_diag_test_message_length_type enum_type;
		uint32_t uint32_type;
	}length_type;
	uint32_t custom_message_length_type; // min 1 max 120(?)
	union
	{
		enum_admin_diag_test_message_body_type enum_type;
		uint32_t uint32_type;
	}message_body_type;
	uint32_t batch_acquire_period_ms; // min 100, max 5000, default 500ms; every tot ms a new batch sample is added to the average
	uint32_t num_batch_samples_for_average; // number of batch samples used for computing the average
	union
	{
		enum_admin_diag_test_average_type enum_type;
		uint32_t uint32_type;
	}average_type;

}__attribute__((__packed__)) type_admin_diag_test_req_start_body;

typedef enum
{
	enum_admin_diag_test_start_retcode_OK = 0,
	enum_admin_diag_test_start_retcode_radio_stack_not_running ,
	enum_admin_diag_test_start_retcode_invalid_batch_acquire_period_ms,
	enum_admin_diag_test_start_retcode_invalid_average_type,
	enum_admin_diag_test_start_retcode_invalid_custom_message_length_type,
	enum_admin_diag_test_start_retcode_invalid_length_type,
	enum_admin_diag_test_start_retcode_invalid_message_body_type,
	enum_admin_diag_test_start_retcode_invalid_num_batch_samples_for_average,
	enum_admin_diag_test_start_retcode_invalid_server_IEEE_address,
	enum_admin_diag_test_start_retcode_unable_to_reset_diag_thread,

	enum_admin_diag_test_start_retcode_numof
}enum_admin_diag_test_start_retcode;

typedef struct _type_admin_diag_test_reply_start_body
{
	union
	{
		enum_admin_diag_test_start_retcode enum_retcode;
		uint32_t uint32_retcode;
	}retcode;
	uint8_t retcode_ascii[256]; // a NULL-termined string describing the return code result, es: "OK" or "CLOSED FOR LUNCH, BACK IN 1 HOUR"
}__attribute__((__packed__)) type_admin_diag_test_reply_start_body;


// STATUS REQ

typedef enum
{
	enum_admin_diag_test_status_req_format_default = 0,
	enum_admin_diag_test_status_req_format_numof
}enum_admin_diag_test_status_req_format;

typedef struct _type_admin_diag_test_req_status_body
{
	union
	{
		enum_admin_diag_test_status_req_format enum_format;
		uint32_t uint32_format;
	}format;
}__attribute__((__packed__)) type_admin_diag_test_req_status_body;

typedef struct _type_admin_diag_test_status_standard_format_body
{
	uint64_t nmsg_tx_OK; // numero di messaggi spediti con successo da inizio test
	uint64_t nmsg_tx_ERR; // numero di messaggi spediti con errore da inizio test
	uint64_t nmsg_rx_OK; // numero di messaggi ricevuti con formato corretto da inizio test
	uint64_t nmsg_rx_ERR; // numero di messaggi ricevuti con formato errato da inizio test
	uint64_t nbytes_tx_OK; //  numero di bytes spediti con successo da inizio test
	uint64_t nbytes_rx_OK; // numero di bytes spediti con successo da inizio test
	float average_num_msg_per_second_tx;
	float average_num_msg_per_second_rx;
	float average_num_bytes_per_second_tx;
	float average_num_bytes_per_second_rx;

}__attribute__((__packed__)) type_admin_diag_test_status_standard_format_body;

typedef struct _type_admin_diag_test_reply_status_body
{
	union
	{
		enum_admin_diag_test_status_req_format enum_format;
		uint32_t uint32_format;
	}format;
	union
	{
		type_admin_diag_test_status_standard_format_body standard_format_body;
	}body;
}__attribute__((__packed__)) type_admin_diag_test_reply_status_body;

// STOP

typedef struct _type_admin_diag_test_req_stop_body
{
	uint32_t unused;
}__attribute__((__packed__)) type_admin_diag_test_req_stop_body;

typedef enum
{
	enum_admin_diag_test_stop_retcode_OK = 0,
	enum_admin_diag_test_stop_retcode_numof
}enum_admin_diag_test_stop_retcode;

typedef struct _type_admin_diag_test_reply_stop_body
{
	union
	{
		enum_admin_diag_test_stop_retcode enum_retcode;
		uint32_t uint32_retcode;
	}retcode;
	uint8_t retcode_ascii[256]; // a NULL-termined string describing the return code result, es: "OK" or "WAS NOT RUNNING"
}__attribute__((__packed__)) type_admin_diag_test_reply_stop_body;



#define def_ASAC_ZigBee_admin_diag_test_req_command_version 0

typedef struct _type_ASAC_admin_diag_test_req
{
	// the operation required
	union
	{
		enum_admin_diag_test_op enum_op;
		uint32_t uint32_op;
	}op;
	union
	{
		type_admin_diag_test_req_start_body start;
		type_admin_diag_test_req_status_body status;
		type_admin_diag_test_req_stop_body stop;
	}body;
}__attribute__((__packed__)) type_ASAC_admin_diag_test_req ;

typedef struct _type_ASAC_admin_diag_test_reply
{
	// the operation required
	union
	{
		enum_admin_diag_test_op enum_op;
		uint32_t uint32_op;
	}op;
	union
	{
		type_admin_diag_test_reply_start_body start;
		type_admin_diag_test_reply_status_body status;
		type_admin_diag_test_reply_stop_body stop;
	}body;
}__attribute__((__packed__)) type_ASAC_admin_diag_test_reply ;


//
// enum_ASAC_ZigBee_interface_command_administrator_diagnostic_test ends here
//



//
//
// enum_ASAC_ZigBee_interface_command_network_probe begins here
//
//

#define def_ASAC_ZigBee_network_probe_req_command_version 0

typedef enum
{
	enum_network_probe_op_discovery = 0,
	enum_network_probe_op_numof
}enum_network_probe_op;

typedef struct _type_ASAC_ZigBee_interface_network_probe_request
{
	union
	{
		enum_network_probe_op enum_op;
		uint32_t uint_op;
	}op;
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_probe_request;

typedef struct _type_ASAC_ZigBee_interface_network_probe_reply_discovery
{
	uint32_t serial_number;
	uint64_t uptime_seconds;
	type_ASAC_ZigBee_interface_network_my_IEEE_reply IEEE_address_info;
	type_ASAC_ZigBee_interface_network_firmware_version_reply ASACZ_version;
	type_fwupd_CC2650_read_version_reply_body CC2650_version;
	uint8_t OpenWrt_release[1024];
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_probe_reply_discovery;


typedef struct _type_ASAC_ZigBee_interface_network_probe_reply
{
	union
	{
		enum_network_probe_op enum_op;
		uint32_t uint_op;
	}op;
	union
	{
		type_ASAC_ZigBee_interface_network_probe_reply_discovery discovery;
	}body;
}__attribute__((__packed__)) type_ASAC_ZigBee_interface_network_probe_reply;


//
//
// enum_ASAC_ZigBee_interface_command_network_probe ends here
//
//


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_device_info begins here
//
//
//
#define def_administrator_device_info_command_version 0

typedef enum
{
	enum_administrator_device_info_op_get_serial_number,
	enum_administrator_device_info_op_set_serial_number,
	enum_administrator_device_info_op_numof
}enum_administrator_device_info_op;

typedef struct _type_administrator_device_info_op_get_serial_number
{
	uint32_t unused;
}__attribute__((__packed__))  type_administrator_device_info_op_get_serial_number;

typedef struct _type_administrator_device_info_op_set_serial_number
{
	uint32_t new_serial_number;
	uint32_t key_validate;	// a key to validate the set operation; if the key is incorrect, the operation is not executed
}__attribute__((__packed__))  type_administrator_device_info_op_set_serial_number;

typedef struct _type_ASAC_ZigBee_interface_administrator_device_info_req
{
	union
	{
		enum_administrator_device_info_op enum_op;
		uint32_t uint_op;
	}op;
	union
	{
		type_administrator_device_info_op_get_serial_number get_serial_number;
		type_administrator_device_info_op_set_serial_number set_serial_number;
	}body;
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_device_info_req;

typedef enum
{
	enum_administrator_device_info_get_sn_retcode_OK = 0,
	enum_administrator_device_info_get_sn_retcode_sn_not_set,
	enum_administrator_device_info_get_sn_retcode_numof
}enum_administrator_device_info_get_sn_retcode;
typedef struct _type_administrator_device_info_op_get_serial_number_reply
{
	uint32_t is_valid;
	uint32_t serial_number;
	union
	{
		enum_administrator_device_info_get_sn_retcode enum_retcode;
		uint32_t uint_retcode;
	}retcode;
	uint8_t retcode_ascii[128];
}__attribute__((__packed__))  type_administrator_device_info_op_get_serial_number_reply;

typedef enum
{
	enum_administrator_device_info_set_sn_retcode_OK = 0,
	enum_administrator_device_info_set_sn_retcode_unable_to_write,
	enum_administrator_device_info_set_sn_retcode_invalid_key,
	enum_administrator_device_info_set_sn_retcode_unable_to_calc_key,
	enum_administrator_device_info_set_sn_retcode_error_written_value_differs,
	enum_administrator_device_info_set_sn_retcode_numof
}enum_administrator_device_info_set_sn_retcode;
typedef struct _type_administrator_device_info_op_set_serial_number_reply
{
	uint32_t is_valid;
	uint32_t serial_number;
	union
	{
		enum_administrator_device_info_set_sn_retcode enum_retcode;
		uint32_t uint_retcode;
	}retcode;
	uint8_t retcode_ascii[128];
}__attribute__((__packed__))  type_administrator_device_info_op_set_serial_number_reply;

typedef struct _type_ASAC_ZigBee_interface_administrator_device_info_reply
{
	union
	{
		enum_administrator_device_info_op enum_op;
		uint32_t uint_op;
	}op;
	union
	{
		type_administrator_device_info_op_get_serial_number_reply get_serial_number;
		type_administrator_device_info_op_set_serial_number_reply set_serial_number;
	}body;
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_device_info_reply;


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_device_info ends here
//
//
//



//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_system_reboot begins here
//
//
//
#define def_administrator_system_reboot_command_version 0


typedef struct _type_ASAC_ZigBee_interface_administrator_system_reboot_req
{
	uint32_t reboot_req_id;				// the boot identifier, you can set it to 0
	uint8_t reboot_req_message[64]; 	// a message that's showing up in the system log when requesting the reboot"
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_system_reboot_req;

typedef struct _type_ASAC_ZigBee_interface_administrator_system_reboot_reply
{
	uint32_t reboot_req_id;				// same as per the request
	uint8_t reboot_req_message[64]; 	// same as per the request
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_system_reboot_reply;


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_reboot ends here
//
//
//



//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_restart_me begins here
//
//
//
#define def_administrator_restart_me_command_version 0


typedef struct _type_ASAC_ZigBee_interface_administrator_restart_me_req
{
	uint32_t restart_req_id;				// the restart identifier, you can set it to 0
	uint8_t restart_req_message[64]; 		// a message that's showing up in the system log when requesting the restart
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_restart_me_req;

typedef struct _type_ASAC_ZigBee_interface_administrator_restart_me_reply
{
	uint32_t restart_req_id;				// same as per the request
	uint8_t restart_req_message[64]; 		// same as per the request
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_restart_me_reply;


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_restart_me ends here
//
//
//


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_UTC begins here
//
//
//
#define def_administrator_UTC_command_version 0

typedef enum
{
	enum_UTC_op_get = 0,
	enum_UTC_op_set ,

	enum_UTC_op_numof
}enum_UTC_op;
typedef enum
{
	enum_UTC_op_retcode_OK = 0,
	enum_UTC_op_retcode_ERROR_get ,
	enum_UTC_op_retcode_ERROR_set ,
	enum_UTC_op_retcode_ERROR_unknown_op ,

	enum_UTC_op_retcode_numof
}enum_UTC_op_retcode;

typedef struct _type_ASAC_ZigBee_interface_administrator_UTC_req
{
	// the operation required (basically, get or set), we can see this field as an enum or as an uint32_t
	union
	{
		enum_UTC_op enum_op;
		uint32_t uint_op;
	}op;
	// the time to set, used only when a set operation is required
	union
	{
		int64_t time_to_set;
	}body;
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_UTC_req;

typedef struct _type_ASAC_ZigBee_interface_administrator_UTC_reply
{
	// the operation required
	union
	{
		enum_UTC_op enum_op;
		uint32_t uint_op;
	}op;
	// the return code: can be OK (0), or some error
	union
	{
		enum_UTC_op_retcode enum_r;
		uint32_t uint_r;
	}return_code;
	uint8_t return_ascii[128]; // the return code as ASCII string
	int64_t current_system_epoch_time;	// the current system epoch time
} __attribute__((__packed__)) type_ASAC_ZigBee_interface_administrator_UTC_reply;


//
//
//
// enum_ASAC_ZigBee_interface_command_administrator_UTC ends here
//
//
//


//
//
//
// The ZigBee interface command structure begins here
//
//
//

typedef struct _type_ASAC_Zigbee_interface_request
{
	type_ASAC_Zigbee_interface_header h; // the common header, containing informations about the protocol and the command
	// the union of all of the possibles requests from the clients
	union
	{
		// outside messages requests
		type_ASAC_ZigBee_interface_command_outside_send_message_req outside_send_message;

		// network messages requests
		type_ASAC_ZigBee_interface_network_input_cluster_register_req input_cluster_register;
		type_ASAC_ZigBee_interface_network_input_cluster_unregister_req input_cluster_unregister;
		type_ASAC_ZigBee_interface_network_echo_req echo;
		type_ASAC_ZigBee_interface_network_device_list_req device_list;
		type_ASAC_ZigBee_interface_network_firmware_version_req firmware_version;
		type_ASAC_ZigBee_interface_network_my_IEEE_req my_IEEE;
		type_ASAC_ZigBee_interface_network_signal_strength_req signal_strength;
		type_ASAC_ZigBee_interface_restart_network_from_scratch_req restart_network_from_scratch_req;
		type_ASAC_ZigBee_interface_network_probe_request network_probe_request;
		type_ASAC_ZigBee_interface_administrator_restart_me_req restart_me_req;

		// administration
		type_ASAC_ZigBee_interface_command_fwupd_req fwupd_req;
		type_ASAC_admin_diag_test_req diag_test_req;
		type_ASAC_ZigBee_interface_administrator_device_info_req device_info_req;
		type_ASAC_ZigBee_interface_administrator_system_reboot_req system_reboot_req;
		type_ASAC_ZigBee_interface_administrator_UTC_req UTC_req;

	}req;
}type_ASAC_Zigbee_interface_request;


typedef struct _type_ASAC_Zigbee_interface_command_reply
{
	type_ASAC_Zigbee_interface_header h; // the common header, containing informations about the protocol and the command
	// the union of all of the possibles replies of the server to the clients requests
	union
	{
		// outside messages replies
		type_ASAC_ZigBee_interface_command_outside_send_message_reply outside_send_message;
		// incoming message callback
		type_ASAC_ZigBee_interface_command_received_message_callback received_message_callback;

		// network messages replies
		type_ASAC_ZigBee_interface_network_input_cluster_register_reply input_cluster_register;
		type_ASAC_ZigBee_interface_network_input_cluster_unregister_reply input_cluster_unregister;
		type_ASAC_ZigBee_interface_network_echo_reply echo;
		type_ASAC_ZigBee_interface_network_device_list_reply device_list;
		type_ASAC_ZigBee_interface_network_firmware_version_reply firmware_version;
		type_ASAC_ZigBee_interface_network_device_list_changed_signal device_list_changed;
		type_ASAC_ZigBee_interface_network_my_IEEE_reply my_IEEE;
		type_ASAC_ZigBee_interface_network_signal_strength_reply signal_strength;
		type_ASAC_ZigBee_interface_restart_network_from_scratch_reply restart_network_from_scratch_reply;
		type_ASAC_ZigBee_interface_network_probe_reply network_probe_reply;

		// administration commands
		type_ASAC_ZigBee_interface_command_fwupd_reply fwupd_reply;
		type_ASAC_admin_diag_test_reply diag_test_reply;
		type_ASAC_ZigBee_interface_administrator_device_info_reply device_info_reply;
		type_ASAC_ZigBee_interface_administrator_system_reboot_reply system_reboot_reply;
		type_ASAC_ZigBee_interface_administrator_UTC_reply UTC_reply;
		type_ASAC_ZigBee_interface_administrator_restart_me_reply restart_me_reply;

		// reply to an unknown command
		type_ASAC_ZigBee_interface_unknown_reply unknown;
	}reply;
}type_ASAC_Zigbee_interface_command_reply;

#define def_size_ASAC_Zigbee_interface_code(p_the_struct) (sizeof(p_the_struct->code))

#define def_size_ASAC_Zigbee_interface_req(p_the_struct,the_command_body) (sizeof((p_the_struct)->h) + sizeof((p_the_struct)->req.the_command_body))
#define def_size_ASAC_Zigbee_interface_reply(p_the_struct,the_command_body) (sizeof((p_the_struct)->h) + sizeof((p_the_struct)->reply.the_command_body))

//
//
//
// The ZigBee interface command structure ends here
//
//
//

#endif /* INC_ASAC_ZIGBEE_NETWORK_COMMANDS_H_ */
