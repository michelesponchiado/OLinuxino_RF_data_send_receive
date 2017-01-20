/*
 * ASACZ_message_history.c
 *
 *  Created on: Nov 7, 2016
 *      Author: michele
 */

#include "../inc/ASACZ_message_history.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <time.h>       /* time_t, time (for timestamp in second) */
#include <sys/timeb.h>  /* ftime, timeb (for timestamp in millisecond) */
#include <sys/time.h>   /* gettimeofday, timeval (for timestamp in microsecond) */


#define def_ASACZ_message_tx_history_numof 128
static type_ASACZ_message_tx_history ASACZ_message_tx_history[def_ASACZ_message_tx_history_numof];
typedef struct _type_ASACZ_message_tx_history_handle
{
	pthread_mutex_t mtx; 			//!< the mutex to access the list
	int32_t	aging_index;	//!< useful to find the oldest message in list
	uint32_t num_duplicate_ID_found;
	uint32_t num_suspicious_already_valid_Trans_ID_found;
}type_ASACZ_message_tx_history_handle;
static type_ASACZ_message_tx_history_handle ASACZ_message_tx_history_handle;

void message_history_tx_init(void)
{
	memset(&ASACZ_message_tx_history[0], 0, sizeof(ASACZ_message_tx_history));
	pthread_mutex_init(&ASACZ_message_tx_history_handle.mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&ASACZ_message_tx_history_handle.mtx, &mutexattr);
	}
}

static void add_timestamp(type_ASACZ_message_tx_history * p, enum_message_history_status s)
{
	if (s >= enum_message_history_status_numof)
	{
		s = enum_message_history_status_unknown;
	}
	struct timeb timer_msec;
	long long int timestamp_msec; // timestamp in milliseconds
	if (!ftime(&timer_msec))
	{
		timestamp_msec = ((long long int) timer_msec.time) * 1000ll +	(long long int) timer_msec.millitm;
	}
	else
	{
		timestamp_msec = -1;
	}
	p->timestamp_milliseconds[s] = timestamp_msec;
}


void message_history_tx_add_id(uint32_t message_id, enum_message_history_status status)
{
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = NULL;
		type_ASACZ_message_tx_history * p_oldest = &ASACZ_message_tx_history[0];
		register int32_t aging_index = ASACZ_message_tx_history_handle.aging_index;
		int32_t max_age_index = abs(aging_index - p_oldest->aging_index);
		unsigned int i;
		for (i = 0; !p_found && (i < def_ASACZ_message_tx_history_numof); i++)
		{
			type_ASACZ_message_tx_history * p = &ASACZ_message_tx_history[i];
			if (!p->running || p->ID == message_id)
			{
				p_found = p;
			}
			else
			{
				int32_t cur_age_index = abs(aging_index - p->aging_index);
				if (cur_age_index > max_age_index)
				{
					max_age_index = cur_age_index;
					p_oldest = p;
				}
			}
		}
		if (!p_found)
		{
			p_found = p_oldest;
		}
		memset(p_found, 0, sizeof(*p_found));
		add_timestamp(p_found, enum_message_history_status_just_created);

		p_found->ID = message_id;
		p_found->aging_index = ASACZ_message_tx_history_handle.aging_index++;
		p_found->status = status;
		p_found->running= 1;
		add_timestamp(p_found, status);
		syslog(LOG_INFO, "Created message ID %u, creation time stamp %lld", message_id, p_found->timestamp_milliseconds[enum_message_history_status_just_created]);
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
}

static type_ASACZ_message_tx_history *message_history_tx_find_id(uint32_t message_id)
{
	type_ASACZ_message_tx_history * p_found = NULL;
	unsigned int i;
	for (i = 0; !p_found && (i < def_ASACZ_message_tx_history_numof); i++)
	{
		type_ASACZ_message_tx_history * p = &ASACZ_message_tx_history[i];
		if (p->ID == message_id)
		{
			p_found = p;
		}
	}
	return p_found;
}


enum_message_history_tx_set_id_status_retcode message_history_tx_set_id_status(uint32_t message_id, enum_message_history_status status)
{
	enum_message_history_tx_set_id_status_retcode r = enum_message_history_tx_set_id_status_retcode_OK;
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = message_history_tx_find_id(message_id);
		if (!p_found)
		{
			r = enum_message_history_tx_set_id_status_retcode_ERR_id_not_found;
		}
		else
		{
			p_found->status = status;
			add_timestamp(p_found, status);
		}
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
	return r;
}

static void set_error(type_ASACZ_message_tx_history * p, enum_message_history_error e)
{
	p->error = e;
	if (e != enum_message_history_error_none && p->status != enum_message_history_status_closed_by_ERR)
	{
		enum_message_history_status status = enum_message_history_status_closed_by_ERR;
		p->status = status;
		p->Trans_ID_valid = 0;
		p->running = 0;
		add_timestamp(p, status);
	}
}

enum_message_history_tx_set_error_retcode message_history_tx_set_error(uint32_t message_id, enum_message_history_error e)
{
	enum_message_history_tx_set_id_status_retcode r = enum_message_history_tx_set_id_status_retcode_OK;
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = message_history_tx_find_id(message_id);
		if (!p_found)
		{
			r = enum_message_history_tx_set_error_retcode_ERR_id_not_found;
		}
		else
		{
			set_error(p_found, e);
		}
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
	return r;
}

enum_message_history_tx_set_trans_id_retcode message_history_tx_set_trans_id(uint32_t message_id, uint8_t trans_id)
{
	enum_message_history_tx_set_error_retcode r = enum_message_history_tx_set_trans_id_retcode_OK;
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = NULL;
		unsigned int i;
		for (i = 0; (i < def_ASACZ_message_tx_history_numof); i++)
		{
			type_ASACZ_message_tx_history * p = &ASACZ_message_tx_history[i];
			if (p->running)
			{
				if (p->ID == message_id)
				{
					if (!p_found)
					{
						p_found = p;
						if (p->Trans_ID_valid)
						{
							syslog(LOG_ERR, "Removing duplicate transaction ID %u, creation time stamp %lld", (unsigned int) trans_id, p->timestamp_milliseconds[enum_message_history_status_just_created]);
							ASACZ_message_tx_history_handle.num_suspicious_already_valid_Trans_ID_found++;
						}
						else
						{
							p->Trans_ID_valid = 1;
						}
						p->Trans_ID = trans_id;
						p->status = enum_message_history_status_Trans_ID_assigned;
						add_timestamp(p, p->status);
					}
					else
					{
						syslog(LOG_ERR, "Removing duplicate message ID %u, creation time stamp %lld", message_id, p->timestamp_milliseconds[enum_message_history_status_just_created]);
						ASACZ_message_tx_history_handle.num_duplicate_ID_found++;
						// the message is no more valid because it have the same message ID found before!
						set_error(p, enum_message_history_error_duplicate_message_id);
						// mark the message as no more valid
						p->running = 0;
					}
				}
				// if there is a message with the same transaction ID, better to remove the transaction ID from the history to avoid confusion
				else if (p->Trans_ID_valid && p->Trans_ID == trans_id)
				{
					syslog(LOG_ERR, "Removing duplicate transaction ID %u, creation time stamp %lld", (unsigned int) trans_id, p->timestamp_milliseconds[enum_message_history_status_just_created]);
					// the message is no more valid because it have the same transaction ID as the message ID we are checking in!
					set_error(p, enum_message_history_error_duplicate_Trans_id);
					// mark the transaction ID as no more valid
					p->Trans_ID_valid = 0;
					// mark the message as no more valid
					p->running = 0;
				}
			}
		}
		if (!p_found)
		{
			r = enum_message_history_tx_set_trans_id_retcode_unable_to_find_message_ID;
		}
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
	return r;

}

enum_message_history_tx_set_trans_id_status_retcode message_history_tx_set_trans_id_status(uint8_t TransId, enum_message_history_status s)
{
	enum_message_history_tx_set_trans_id_status_retcode r = enum_message_history_tx_set_trans_id_status_retcode_OK;
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = NULL;
		unsigned int i;
		for (i = 0; (i < def_ASACZ_message_tx_history_numof); i++)
		{
			type_ASACZ_message_tx_history * p = &ASACZ_message_tx_history[i];
			if (p->running)
			{
				if (p->Trans_ID_valid && p->Trans_ID == TransId)
				{
					p_found = p;
					p->status = s;
					add_timestamp(p, p->status);
					// if we have received the data confirm, the transaction ID has been used and it is no more valid
					// AND FURTHERMORE, THE MESSAGE IS COMPLETED OK
					if (s == enum_message_history_status_dataconfirm_received)
					{
						p->Trans_ID_valid = 0;
						p->status = enum_message_history_status_closed_OK;
						p->running = 0;
						add_timestamp(p, p->status);
					}
				}
			}
		}
		if (!p_found)
		{
			r = enum_message_history_tx_set_trans_id_status_retcode_transid_not_found;
		}
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
	return r;
}

enum_message_history_tx_set_trans_id_error_retcode message_history_tx_set_trans_id_error(uint8_t TransId, enum_message_history_error e)
{
	enum_message_history_tx_set_trans_id_error_retcode r = enum_message_history_tx_set_trans_id_error_retcode_OK;
	pthread_mutex_lock(&ASACZ_message_tx_history_handle.mtx);
		type_ASACZ_message_tx_history * p_found = NULL;
		unsigned int i;
		for (i = 0; (i < def_ASACZ_message_tx_history_numof); i++)
		{
			type_ASACZ_message_tx_history * p = &ASACZ_message_tx_history[i];
			if (p->running)
			{
				if (p->Trans_ID_valid && p->Trans_ID == TransId)
				{
					p_found = p;
					set_error(p_found, e);
				}
			}
		}
		if (!p_found)
		{
			r = enum_message_history_tx_set_trans_id_status_retcode_transid_not_found;
		}
	pthread_mutex_unlock(&ASACZ_message_tx_history_handle.mtx);
	return r;
}


