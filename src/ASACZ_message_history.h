/*
 * ASACZ_message_history.h
 *
 *  Created on: Nov 7, 2016
 *      Author: michele
 */

#ifndef ASACZ_MESSAGE_HISTORY_H_
#define ASACZ_MESSAGE_HISTORY_H_
#include <stdint.h>


typedef enum
{
	enum_message_history_error_none = 0,
	enum_message_history_error_DATAREQ,
	enum_message_history_error_DATACONFIRM,
	enum_message_history_error_TIMEOUT_DATACONFIRM,
	enum_message_history_error_duplicate_message_id,
	enum_message_history_error_duplicate_Trans_id,
	enum_message_history_error_too_big_outside_message_on_pop,
	enum_message_history_error_unknown_IEEE_address,
	enum_message_history_error_unable_to_send_DATAREQ,
	enum_message_history_error_numof,
}enum_message_history_error;
typedef enum
{
	enum_message_history_status_idle = 0,
	enum_message_history_status_just_created,
	enum_message_history_status_in_waiting_queue,
	enum_message_history_status_Trans_ID_assigned,
	enum_message_history_status_datareq_sent,
	enum_message_history_status_dataconfirm_received,
	enum_message_history_status_closed_OK,
	enum_message_history_status_closed_by_ERR,
	enum_message_history_status_unknown,
	enum_message_history_status_numof
}enum_message_history_status;
typedef struct _type_ASACZ_message_tx_history
{
	unsigned int running;		// the message is running
	unsigned int ID;			// the ID of the message
	int32_t	aging_index;	//!< useful to find the oldest message in list
	// http://zigbeewiki.anaren.com/index.php/AF_DATA_CONFIRM
	// If AF_ACK_REQUEST is NOT used (default): then AF_DATA_CONFIRM sent when the next node on the route acknowledges the message.
    // If AF_ACK_REQUEST is used: then AF_DATA_CONFIRM sent when the final destination node acknowledges the message.
	enum_message_history_status status;
	enum_message_history_error error;	// the error type
	unsigned int Trans_ID_valid;			// the Trans_ID has been assigned and is still valid
	uint8_t Trans_ID;			// the transaction ID of the message
	long long int timestamp_milliseconds[enum_message_history_status_numof]; // the time stamp of the message in milliseconds, in the various states
}type_ASACZ_message_tx_history;

// add the id to the message history
void message_history_tx_add_id(uint32_t message_id, enum_message_history_status status);
typedef enum
{
	enum_message_history_tx_set_id_status_retcode_OK,
	enum_message_history_tx_set_id_status_retcode_ERR_id_not_found,
	enum_message_history_tx_set_id_status_retcode_numof
}enum_message_history_tx_set_id_status_retcode;
enum_message_history_tx_set_id_status_retcode message_history_tx_set_id_status(uint32_t message_id, enum_message_history_status status);

typedef enum
{
	enum_message_history_tx_set_error_retcode_OK,
	enum_message_history_tx_set_error_retcode_ERR_id_not_found,
	enum_message_history_tx_set_error_retcode_numof
}enum_message_history_tx_set_error_retcode;
enum_message_history_tx_set_error_retcode message_history_tx_set_error(uint32_t message_id, enum_message_history_error e);

typedef enum
{
	enum_message_history_tx_set_trans_id_retcode_OK = 0,
	enum_message_history_tx_set_trans_id_retcode_unable_to_find_message_ID,
	enum_message_history_tx_set_trans_id_retcode_numof
}enum_message_history_tx_set_trans_id_retcode;
enum_message_history_tx_set_trans_id_retcode message_history_tx_set_trans_id(uint32_t message_id, uint8_t trans_id);

typedef enum
{
	enum_message_history_tx_set_trans_id_status_retcode_OK,
	enum_message_history_tx_set_trans_id_status_retcode_transid_not_found,
	enum_message_history_tx_set_trans_id_status_retcode_numof
}enum_message_history_tx_set_trans_id_status_retcode;

enum_message_history_tx_set_trans_id_status_retcode message_history_tx_set_trans_id_status(uint8_t TransId, enum_message_history_status s);
typedef enum
{
	enum_message_history_tx_set_trans_id_error_retcode_OK,
	enum_message_history_tx_set_trans_id_error_retcode_transid_not_found,
	enum_message_history_tx_set_trans_id_error_retcode_numof
}enum_message_history_tx_set_trans_id_error_retcode;
enum_message_history_tx_set_trans_id_error_retcode message_history_tx_set_trans_id_error(uint8_t TransId, enum_message_history_error e);


#endif /* ASACZ_MESSAGE_HISTORY_H_ */
