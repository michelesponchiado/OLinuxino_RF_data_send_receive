/*
 * AF_register.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_AF_register.h"


static const RegisterFormat_t default_RegisterFormat_t =
{
		.AppProfId = 0x0104,
		.AppDeviceId = 0x0100,
		.AppDevVer = 1,
		.LatencyReq = 0,
		.EndPoint = 0xf0,
		.AppNumInClusters = 1,
		.AppInClusterList[0] = 0x0001,
		.AppNumOutClusters = 1,
		.AppOutClusterList[0] = 0x0001,
};


int32_t registerAf_default(void)
{
	int32_t status = 0;
	RegisterFormat_t reg;
	memcpy(&reg, &default_RegisterFormat_t, sizeof(reg));

	status = afRegister(&reg);
	return status;
}

unsigned int is_OK_registerAf_user(type_Af_user *p)
{
	unsigned int is_OK = 1;
	if (is_OK)
	{
		if (p->AppNumInClusters > sizeof(p->AppInClusterList)/sizeof(p->AppInClusterList[0]))
		{
			is_OK = 0;
		}
	}
	if (is_OK)
	{
		if (p->AppNumOutClusters > sizeof(p->AppOutClusterList)/sizeof(p->AppOutClusterList[0]))
		{
			is_OK = 0;
		}
	}
	if (is_OK)
	{

		my_log(1,"%s: refreshing my point list @end point %u...", __func__, (unsigned int)p->EndPoint);
		RegisterFormat_t reg;
		memcpy(&reg, &default_RegisterFormat_t, sizeof(reg));
		reg.EndPoint = p->EndPoint;
		{
			reg.AppNumInClusters = p->AppNumInClusters;
			int i;
			for (i =0; i < reg.AppNumInClusters; i++)
			{
				reg.AppInClusterList[i] = p->AppInClusterList[i];
			}
		}
		{
			reg.AppNumOutClusters = p->AppNumOutClusters;
			int i;
			for (i =0; i < reg.AppNumOutClusters; i++)
			{
				reg.AppOutClusterList[i] = p->AppOutClusterList[i];
			}
		}

		{
			int32_t status = 0;
			my_log(1,"%s: about to call register af user", __func__, (unsigned int)p->EndPoint);
			status = afRegister(&reg);
			if (status != SUCCESS)
			{
				my_log(1,"%s: error afregister", __func__);
				is_OK = 0;
			}
			else
				my_log(1,"%s: OK afregister", __func__);
		}

	}
	{
		if (is_OK)
		{
			my_log(1,"%s: ends OK", __func__);
		}
		else
		{
			my_log(1,"%s: ERROR, ends non OK", __func__);
		}

	}
	return is_OK;
}

uint8_t get_default_dst_endpoint(void)
{
	return default_RegisterFormat_t.EndPoint;
}
uint8_t get_default_src_endpoint(void)
{
	return default_RegisterFormat_t.EndPoint;
}
uint16_t get_default_clusterID (void)
{
	return default_RegisterFormat_t.AppInClusterList[0];
}


