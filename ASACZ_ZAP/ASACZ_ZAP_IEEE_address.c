/*
 * IEEE_address.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_IEEE_address.h"


void invalidate_IEEE_address(type_handle_app_IEEE_address *p)
{
	memset(p, 0, sizeof(*p));
}
void set_IEEE_address(type_handle_app_IEEE_address *p, uint64_t address)
{
	p->address = address;
	p->valid = 1;
}
uint64_t get_IEEE_address(type_handle_app_IEEE_address *p)
{
	if (p->valid)
	{
		return p->address;
	}
	return 0;
}
unsigned int is_valid_IEEE_address(type_handle_app_IEEE_address *p)
{
	return p->valid;
}

uint32_t is_OK_get_my_radio_IEEE_address(uint64_t *p)
{
	volatile type_handle_app_IEEE_address ieee_1;
	volatile type_handle_app_IEEE_address ieee_2;
	unsigned int i;
	unsigned int found = 0;
	for (i = 0; i < 20; i++)
	{
		ieee_1 = handle_app.IEEE_address;
		ieee_2 = handle_app.IEEE_address;
		if(memcmp((const void *)&ieee_1, (const void *)&ieee_2, sizeof(ieee_1)) == 0)
		{
			found = 1;
			break;
		}
	}
	if (!found || !is_valid_IEEE_address((type_handle_app_IEEE_address *)&ieee_1))
	{
		return 0;
	}
	*p = ieee_1.address;
	return 1;
}


