/*
 * ASACZ_devices_list.h
 *
 *  Created on: Nov 4, 2016
 *      Author: michele
 */

#ifndef ASACZ_DEVICES_LIST_H_
#define ASACZ_DEVICES_LIST_H_
#include <stdint.h>
/**
 * Z-stack monitor and test API
 * 3.12.2.24 ZDO_END_DEVICE_ANNCE_IND
 * Attribute	Length (byte)	Description
 *
 * SrcAddr 		2 				Source address of the message.
 * NwkAddr 		2 				Specifies the device’s short address.
 * IEEEAddr 	8 				Specifies the 64 bit IEEE address of source device.
 * Capabilities	1				Specifies the MAC capabilities of the device.
 * 		Bit:
 * 		0 – Alternate PAN Coordinator
 * 		1 – Device type: 1- ZigBee Router; 0 – End Device
 * 		2 – Power Source: 1 Main powered
 * 		3 – Receiver on when Idle
 * 		4 – Reserved
 * 		5 – Reserved
 * 		6 – Security capability
 * 		7 – Reserved
 *
 */
typedef struct _type_struct_ASACZ_MAC_capabilities
{
	unsigned int alternate_PAN_coordinator:1; 		// 0 – Alternate PAN Coordinator
	unsigned int is_ZigBeeRouter1_EndDevice0:1;	// 1 – Device type: 1- ZigBee Router; 0 – End Device
	unsigned int is_main_powered:1;				// 2 – Power Source: 1 Main powered
	unsigned int is_receiver_on_when_idle:1;		// 3 – Receiver on when Idle
	unsigned int reserved1:1;						// 4 – Reserved
	unsigned int reserved2:1;						// 5 – Reserved
	unsigned int security_capability:1;			// 6 – Security capability
	unsigned int reserved3:1;						// 7 – Reserved
}type_struct_ASACZ_MAC_capabilities;

typedef union _type_union_ASACZ_MAC_capabilities
{
	uint8_t u8;
	type_struct_ASACZ_MAC_capabilities s;
}type_union_ASACZ_MAC_capabilities;

// please see Z-Stack Monitor and Test API SWRA198 Revision 1.13
// 3.12.2.5 ZDO_SIMPLE_DESC_RSP


#define def_ASACZ_max_clusters_input 16		//!< the maximum number of input clusters as per specifications
#define def_ASACZ_max_clusters_output 16	//!< the maximum number of output clusters as per specifications
/**
 * The end-point structure
 */
typedef struct _type_struct_ASACZ_endpoint_list_element
{
	uint8_t 	end_point;										//!< the end-point index
	uint16_t 	profile_ID;										//!< the profile identifier
	uint16_t 	device_ID;										//!< the device identifier
	uint8_t 	device_version; 								//!< 0--> version 1.00
	uint8_t 	num_of_clusters_input;									//!< the number of input  clusters
	uint16_t 	list_clusters_input[def_ASACZ_max_clusters_input];		//!< the list   of input  clusters
	uint8_t 	num_of_clusters_output;									//!< the number of output clusters
	uint16_t 	list_clusters_output[def_ASACZ_max_clusters_output];	//!< the list   of output clusters
}type_struct_ASACZ_endpoint_list_element;

// please see 3.12.2.6 ZDO_ACTIVE_EP_RSP
#define def_ASACZ_max_end_points 77	//!< the maximum number of end-points defined, as per specifications
/**
 * The end-point list
 */
typedef struct _type_struct_ASACZ_end_point_list
{
	uint8_t 	num_of_active_end_points;	//!< the number of active end-points
	type_struct_ASACZ_endpoint_list_element list[def_ASACZ_max_end_points];	//!< the list of the active end-points of the device
}type_struct_ASACZ_end_point_list;

/**
 * The device header
 */
typedef struct _type_struct_ASACZ_device_header
{
	uint16_t	network_short_address;		//!< the short network address of the device
	uint64_t	IEEE_address;				//!< the IEEE 64-bit address of the device
	type_struct_ASACZ_MAC_capabilities MAC_capabilities;	//!< the MAC capabilities
}type_struct_ASACZ_device_header;
/**
 * The device life cycle information
 */
typedef struct _type_struct_ASACZ_device_lifecycle
{
	int64_t put_in_list_epoch_time_ms;
	int64_t last_msg_rx_epoch_time_ms;
}type_struct_ASACZ_device_lifecycle;

/**
 * An element in the device list
 */
typedef struct _type_struct_ASACZ_device_list_element
{
	type_struct_ASACZ_device_header device_header;
	type_struct_ASACZ_device_lifecycle lifecycle;
	type_struct_ASACZ_end_point_list end_point_list;	//!< the structure of the active end-points of the device
}type_struct_ASACZ_device_list_element;

// maximum number of devices in list
#define def_ASACZ_max_devices_in_list 128
/**
 * The device list structure
 */
typedef struct _type_struct_ASACZ_device_list
{
	uint32_t	num_of_devices;		//!< the number of devices in list
	type_struct_ASACZ_device_list_element list[def_ASACZ_max_devices_in_list];	//!< the list of devices
}type_struct_ASACZ_device_list;

/**
 * Initialize the device list structure
 */
void init_ASACZ_device_list(void);

/**
 * returns the network short address form the IEEE address
 */
unsigned int is_OK_get_network_short_address_from_IEEE(uint64_t IEEE_address, uint16_t * p_network_short_address);

typedef enum
{
	enum_device_lifecycle_action_do_nothing = 0,
	enum_device_lifecycle_action_do_refresh_rx,
	enum_device_lifecycle_action_numof
}enum_device_lifecycle_action;
/**
 * returns the IEEE address form the network short address
 */
unsigned int is_OK_get_IEEE_from_network_short_address(uint16_t network_short_address, uint64_t *p_IEEE_address, enum_device_lifecycle_action e);
/**
 * Adds/resets a device in the list
 */
typedef enum
{
	enum_add_ASACZ_device_list_header_retcode_OK = 0,
	enum_add_ASACZ_device_list_header_retcode_ERR_no_room,
	enum_add_ASACZ_device_list_header_retcode_ERR_unable_to_add,
	enum_add_ASACZ_device_list_header_retcode_numof
}enum_add_ASACZ_device_list_header_retcode;
enum_add_ASACZ_device_list_header_retcode add_ASACZ_device_list_header(type_struct_ASACZ_device_header *p_device_header);

typedef enum
{
	enum_add_ASACZ_device_list_end_points_retcode_OK = 0,
	enum_add_ASACZ_device_list_end_points_retcode_ERR_too_many_end_points,
	enum_add_ASACZ_device_list_end_points_retcode_ERR_network_short_address_not_found,
	enum_add_ASACZ_device_list_end_points_retcode_numof
}enum_add_ASACZ_device_list_end_points_retcode;
enum_add_ASACZ_device_list_end_points_retcode add_ASACZ_device_list_end_points(uint16_t	network_short_address, uint8_t num_of_end_points, uint8_t *list_end_points);

typedef enum
{
	enum_add_ASACZ_device_list_single_end_point_retcode_OK = 0,
	enum_add_ASACZ_device_list_single_end_point_retcode_ERR_network_short_address_not_found,
	enum_add_ASACZ_device_list_end_points_retcode_ERR_end_point_undefined,
	enum_add_ASACZ_device_list_single_end_point_retcode_numof
}enum_add_ASACZ_device_list_single_end_point_retcode;

enum_add_ASACZ_device_list_single_end_point_retcode add_ASACZ_device_list_single_end_point(uint16_t	network_short_address, type_struct_ASACZ_endpoint_list_element *p);

typedef struct _type_walk_ASACZ_device_list
{
	uint32_t num_of_sequence;
	uint32_t offset_before;
	uint32_t offset_after;
	uint32_t bytes_copied;
	uint32_t end_of_walk;
}type_walk_ASACZ_device_list;

typedef enum
{
	enum_walk_ASACZ_device_list_retcode_OK = 0,
	enum_walk_ASACZ_device_list_retcode_ERR_sequence_changed,
	enum_walk_ASACZ_device_list_retcode_numof
}enum_walk_ASACZ_device_list_retcode;
enum_walk_ASACZ_device_list_retcode walk_ASACZ_device_list(type_walk_ASACZ_device_list *p, uint8_t *p_buffer, unsigned int max_bytes);


#define def_max_IEEE_in_list_chunk 128
typedef struct _type_struct_device_list
{
	uint32_t start_index;			//!< the index of the first element in list
	uint32_t sequence; 				//!< the current sequence number
	uint32_t sequence_valid;		//!< this is set to 1 if the request is valid; if not, the client must restart the list request from start_index 0
									//!< because the device list has changed
	uint32_t list_ends_here;		//!< this is set to 1 if the device list has been fully reported, if it is 0, another query starting from start_index+num_devices_in_chunk is needed
	uint32_t num_devices_in_chunk;	//!< the number of devices in the chunk
	uint64_t IEEE_chunk[def_max_IEEE_in_list_chunk];
}type_struct_device_list;

unsigned int is_OK_get_device_IEEE_list(uint32_t start_index, uint32_t sequence, type_struct_device_list *p, uint32_t max_num_of_devices_to_return);

#endif /* ASACZ_DEVICES_LIST_H_ */
