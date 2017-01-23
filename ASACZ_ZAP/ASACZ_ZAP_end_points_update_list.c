/*
 * end_points_update_list.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_end_points_update_list.h"


#define def_period_check_update_end_point_list_ms 200
void reset_time_end_point_update_list(type_handle_end_point_update * p)
{
	p->last_update_epoch_ms = get_current_epoch_time_ms();
}
unsigned int is_needed_end_point_update_list(type_handle_end_point_update * p)
{
	unsigned int update_needed = 0;
	{
		unsigned int do_check = 1;
		int64_t now = get_current_epoch_time_ms();
		if (now > p->last_update_epoch_ms)
		{
			if (now - p->last_update_epoch_ms < def_period_check_update_end_point_list_ms)
			{
				do_check = 0;
			}
		}
		if (do_check)
		{
			reset_time_end_point_update_list(p);
			if (!is_empty_update_end_point_list(get_ptr_to_update_end_point_list()))
			{
				update_needed = 1;
			}
		}
	}
	return update_needed;
}

void handle_end_points_update_list(type_handle_end_point_update * p)
{
	type_update_end_point_list * p_queue = get_ptr_to_update_end_point_list();
	unsigned int i;
	// max 3 update loops
	for (i = 0; i < 3; i++)
	{
		uint8_t end_point_to_update;
		// if there are no end points to update, end of the loop
		if (!is_OK_get_next_update_end_point_list(p_queue, &end_point_to_update))
		{
			break;
		}
		dbg_print(PRINT_LEVEL_INFO,"updating end point %u", (unsigned int)end_point_to_update);
		// increment the number of call back requests
		p->call_back_req ++;
		// send the "delete end point" message
		switch(afDeleteEndpoint(end_point_to_update))
		{
			case MT_RPC_SUCCESS:
			{
				dbg_print(PRINT_LEVEL_VERBOSE,"end point %u deleted OK", (unsigned int)end_point_to_update);
				break;
			}
			default:
			{
				dbg_print(PRINT_LEVEL_ERROR,"error deleting end point %u", (unsigned int)end_point_to_update);
				break;
			}
		}
		// wait for the call back reply
#define def_total_timeout_wait_delete_callback_ms 5000
#define def_base_timeout_wait_delete_callback_ms 20
#define def_num_loop_wait_delete_callback_ms (def_total_timeout_wait_delete_callback_ms / def_base_timeout_wait_delete_callback_ms)
		int max_loops = 1 + def_num_loop_wait_delete_callback_ms;
		unsigned int continue_loop = 1;
		while(continue_loop )
		{
			if (p->call_back_ack == p->call_back_req)
			{
				continue_loop = 0;
				switch(p->call_back_return_code)
				{
					case SUCCESS:
					{
						dbg_print(PRINT_LEVEL_INFO,"OK received from the callback deleting end point %u", (unsigned int)end_point_to_update);
						break;
					}
					case 0x01:
					{
						dbg_print(PRINT_LEVEL_WARNING,"end point list is empty");
						break;
					}
					case 0x02:
					{
						dbg_print(PRINT_LEVEL_WARNING,"end point %u not found in the list", (unsigned int)end_point_to_update);
						break;
					}
					default:
					{
						dbg_print(PRINT_LEVEL_ERROR,"Error code %u from the callback deleting end point %u", (unsigned int)p->call_back_return_code, (unsigned int)end_point_to_update);
						break;
					}
				}
			}
			else if (max_loops > 0)
			{
				max_loops--;
				usleep(def_base_timeout_wait_delete_callback_ms * 1000);
			}
			else
			{
				continue_loop = 0;
				dbg_print(PRINT_LEVEL_ERROR,"timeout waiting for the callback deleting end point %u", (unsigned int)end_point_to_update);
			}
		}
		type_Af_user u;
		// fill-up the u structure containing all the clusters belonging to the selected end point
		if (is_OK_fill_end_point_command_list(end_point_to_update, &u))
		{
			// now we must register the end point sending the appropriate message to the radio module
			if (is_OK_registerAf_user(&u))
			{
				my_log(LOG_INFO,"%s: OK registered end_point %u (%u commands)", __func__, (unsigned int)end_point_to_update, (unsigned int )u.AppNumInClusters);
			}
			else
			{
				my_log(LOG_ERR,"%s: ERROR registering end_point %u (%u commands)", __func__, (unsigned int)end_point_to_update, (unsigned int )u.AppNumInClusters);
			}
		}
		else
		{
			dbg_print(PRINT_LEVEL_ERROR,"unable to fill up the end point %u", (unsigned int)end_point_to_update);
		}

	}
	// reset the update timeout
	reset_time_end_point_update_list(p);
}


void refresh_my_endpoint_list(void)
{
	uint16_t my_short_address;
	my_log(1,"%s: +", __func__);
	if (!is_OK_get_network_short_address_from_IEEE(handle_app.IEEE_address.address, &my_short_address))
	{
		my_log(1,"%s: ERROR unable to refresh my end point list", __func__);
	}
	else
	{
		my_log(1,"%s: refreshing my end point list...", __func__);
		ActiveEpReqFormat_t actReq;
		actReq.DstAddr = my_short_address;
		actReq.NwkAddrOfInterest = my_short_address;
		zdoActiveEpReq(&actReq);
		rpcWaitMqClientMsg(15000); //rpcGetMqClientMsg();
		int32_t status;
		int nloop = 0;
		do
		{
			status = rpcWaitMqClientMsg(1000);
			my_log(1,"%s: status = %i, nloop = %i", __func__, status, nloop);
		} while (status != -1 && (++nloop < 30));
	}
	my_log(1,"%s: -", __func__);
}


void register_user_end_points(void)
{
	uint8_t prev_end_point = 0;
	uint32_t nloop = 0;
	my_log(LOG_INFO, "%s: + registering end_point", __func__);
	while(++nloop < 256)
	{
		type_Af_user u;

		enum_get_next_end_point_command_list_retcode r = get_next_end_point_command_list(prev_end_point, &u);
		if (r != enum_get_next_end_point_command_list_retcode_OK)
		{
			break;
		}
		prev_end_point = u.EndPoint;
		my_log(LOG_INFO,"%s: registering end_point %u (%u commands)", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
		if (!is_OK_registerAf_user(&u))
		{
			my_log(LOG_ERR,"%s: ERROR registering end_point %u (%u commands)", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
		}
		else
		{
			my_log(LOG_INFO,"%s: OK registered end_point %u (%u commands)", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);

		}

	}
	my_log(LOG_INFO,"%s: - registering end_point", __func__);
}
