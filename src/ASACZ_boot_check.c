/*
 * ASACZ_boot_check.c
 *
 *  Created on: Feb 15, 2017
 *      Author: michele
 */


/*
 * main.c
 *
 *  Created on: Feb 15, 2017
 *      Author: michele
 */

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <ASACZ_ZAP.h>
#include <ASACZ_boot_check.h>
#include <stdarg.h>

const uint8_t my_dest_MAC[ETH_ALEN] =
{
	 0xFF
	,0xFF
	,0xFF
	,0xFF
	,0xFF
	,0xFF
};


// default interface name to use
#define DEFAULT_IF	"eth0.1"

// the maximum size of the send buffer on the RAW socket
#define SEND_BUF_SIZE		1024

//
// START
//
// defines setting the maximum time to wait for the boot check startup
//
// max number of seconds trying to open a socket to check if a boot operation is required
#define def_max_wait_start_boot_check_s 120
//
// derived defines, please do not change these!
//
#define def_max_wait_start_boot_check_ms (def_max_wait_start_boot_check_s * 1000)
#define def_pause_between_start_boot_check_ms (5000)
#define def_max_wait_start_boot_check_num_loop (def_max_wait_start_boot_check_ms/def_pause_between_start_boot_check_ms)
// well, at least three tries...
#define def_max_wait_start_boot_check_num_loop_safe (def_max_wait_start_boot_check_num_loop > 3 ? def_max_wait_start_boot_check_num_loop : 3)
//
// END
//
// defines setting the maximum time to wait for the boot check startup
//


//
// START
//
// these defines set the timeouts
//


//#define def_infinite_loop // use this define to wait forever an incoming configuration packet; only when the packet arrives, the boot check ends

// timeout waiting for a reply to boot check request
#define def_timeout_check_boot_message_ms 6000
// delay to wait [seconds] after the network has been reconfigured
#define def_seconds_pause_network_reconfigured 20

// please change the following only if it is really needed!
//
// timeout waiting for a reply to a single boot check request
#define def_timeout_rx_reply_boot_message_ms 1000
// pause between reply checks
#define def_pause_rx_reply_boot_message_ms 100
// number of loop waiting for the reply to the boot check message
#define def_nloop_wait_reply_boot_message_ms (def_timeout_rx_reply_boot_message_ms / def_pause_rx_reply_boot_message_ms)
// max number of tries waiting for a reply to the boot check messages
#define def_max_tries_boot_check (def_timeout_check_boot_message_ms / def_timeout_rx_reply_boot_message_ms)
//
// END
//
// these defines set the timeouts
//

// the checksum of the UDP packet
static unsigned short csum(unsigned short *buf, int nwords)
{
	unsigned long sum;
	for(sum=0; nwords>0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned short)(~sum);
}

// the known ip configuration types
typedef enum
{
	enum_boot_pack_ip_conf_type_DHCP = 1,
	enum_boot_pack_ip_conf_type_static = 2,
}enum_boot_pack_ip_conf_type;

/*
 * DNS
 *  uci delete dhcp.@dnsmasq[-1]
    uci add dhcp dnsmasq
    uci set dhcp.@dnsmasq[-1].server='8.8.8.8'
    uci add_list dhcp.@dnsmasq[-1].server='8.8.8.4'

// gateway
    uci delete dhcp.lan.dhcp_option
    uci add_list dhcp.lan.dhcp_option='3,192.168.0.254'
 *
 */
typedef struct _type_ASACZ_boot_packet
{
	uint8_t mac_address[6];
	uint8_t ip_conf_type[1];	// 1 = DHCP; 2 = static
	uint8_t ip_address[4];		// the ip address to set
	uint8_t subnet_mask[4];		// netmask; 0.0.0.0 = not set
	uint8_t gateway[4];			// use 'uci add_list dhcp.lan.dhcp_option='3,192.168.0.254' uci commit dhcp' to set the default gateway. A list of options can be found here; 0.0.0.0 = not set
	uint8_t dns_primary[4]; 	// uci add_list dhcp.@dnsmasq[-1].server=8.8.8.8  (list server '8.8.8.8'); 0.0.0.0 = not set
	uint8_t dns_alternative[4]; // uci add_list dhcp.@dnsmasq[-1].server=8.8.8.4; 0.0.0.0 = not set
}__attribute__((__packed__)) type_ASACZ_boot_packet;


static const char *boot_check_error_code_message[enum_boot_check_retcode_numof]=
{
	[enum_boot_check_retcode_OK] = "OK",
	[enum_boot_check_retcode_ERR_snprintf] ="ERR snprintf",
	[enum_boot_check_retcode_ERR_socket_open] = "ERR_socket_open",
	[enum_boot_check_retcode_ERR_ioctl_SIOCGIFINDEX] ="ERR_ioctl_SIOCGIFINDEX",
	[enum_boot_check_retcode_ERR_ioctl_SIOCGIFHWADDR] = "ERR_ioctl_SIOCGIFHWADDR",
	[enum_boot_check_retcode_ERR_unable_set_socket_tx_non_blocking] = "ERR_unable_set_socket_tx_non_blocking",
	[enum_boot_check_retcode_ERR_unable_open_socket_rx] = "ERR_unable_open_socket_rx",
	[enum_boot_check_retcode_ERR_unable_read_flags_socket_rx] = "ERR_unable_read_flags_socket_rx",
	[enum_boot_check_retcode_ERR_unable_write_flags_socket_rx] = "ERR_unable_write_flags_socket_rx",
	[enum_boot_check_retcode_ERR_unable_to_send] = "ERR_unable_to_send",
	[enum_boot_check_retcode_ERR_unable_to_bind_socket_rx] = "ERR_unable_to_bind_socket_rx",
	[enum_boot_check_retcode_ERR_snprintf_idx_ifr_name] = "ERR_snprintf_idx_ifr_name",
	[enum_boot_check_retcode_ERR_snprintf_mac_ifr_name] = "ERR_snprintf_mac_ifr_name",
};

const char *get_boot_check_error_code_message(enum_boot_check_retcode r)
{
	const char * p_ret = "UNKNOWN";
	if (r < enum_boot_check_retcode_numof)
	{
		const char * p_new = boot_check_error_code_message[r];
		if (p_new)
		{
			p_ret = p_new;
		}
	}
	return p_ret;
}

// the structure to handle the boot check startup
typedef struct _type_struct_boot_check_run
{
	unsigned int idx_loop_retry_init;
	unsigned int boot_check_initialized_OK;
}type_struct_boot_check_run;

// the structure to build the RAW ethernet message sent to check if a boot configuration change is needed
typedef struct _type_struct_boot_check
{
	int socket_RAW_tx;
	int socket_UDP_rx;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len;
	char sendbuf[SEND_BUF_SIZE];
	struct ether_header *eh;
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
	struct sockaddr_in server_address;

}type_struct_boot_check;

// executes a shell command, please be careful!
static unsigned int is_OK_execute_command (char * format, ...)
{
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	va_list args;
	va_start (args, format);
	if (vsnprintf (buffer, 255, format, args) < 0)
	{
		my_log(LOG_ERR,"%s: unable to print the command %s", __func__, buffer);
		return 0;
	}
	else
	{
		my_log(LOG_INFO,"%s: about to execute command %s", __func__, buffer);
		int ret_value = system(buffer);
		if (ret_value < 0)
		{
			my_log(LOG_ERR,"%s: ERROR executing the command: %i", __func__, ret_value);
			return 0;
		}
		else
		{
			my_log(LOG_INFO,"%s: the return code is %i", __func__, ret_value);
		}
		return 1;
  }

  va_end (args);
}


// initializes the boot check structure
void boot_check_init(type_struct_boot_check *p)
{
	memset(p, 0, sizeof(*p));
	p->eh = (struct ether_header *) p->sendbuf;
}

// executes the boot check
enum_boot_check_retcode boot_check(void)
{
	enum_boot_check_retcode r = enum_boot_check_retcode_OK;
	type_struct_boot_check_run run;
	memset(&run, 0, sizeof(run));

	type_struct_boot_check s_boot_check;
	boot_check_init(&s_boot_check);

	my_log(LOG_INFO,"%s: + starts", __func__);

// loop trying to open a tx RAW socket and an UDP rx socket
// the sockets' open may fail if the server is launched too early, so it seems a good idea to keep trying until timeout or the thing settles OK
	for (run.idx_loop_retry_init = 0; !run.boot_check_initialized_OK && (run.idx_loop_retry_init < def_max_wait_start_boot_check_num_loop_safe); run.idx_loop_retry_init++)
	{
		my_log(LOG_INFO,"%s: init loop %u of %u", __func__, (unsigned int)(run.idx_loop_retry_init + 1), def_max_wait_start_boot_check_num_loop_safe);
		boot_check_init(&s_boot_check);
		r = enum_boot_check_retcode_OK;


		if (r == enum_boot_check_retcode_OK)
		{
			/* Get interface name */
			if (snprintf(s_boot_check.ifName, sizeof(s_boot_check.ifName), DEFAULT_IF) < 0)
			{
				r = enum_boot_check_retcode_ERR_snprintf;
			}
		}

		if (r == enum_boot_check_retcode_OK)
		{
			/* Open RAW socket to send on */
			if ((s_boot_check.socket_RAW_tx = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1)
			{
				r = enum_boot_check_retcode_ERR_socket_open;
			}
		}


		if (r == enum_boot_check_retcode_OK)
		{
			/* Get the index of the interface to send on */
			memset(&s_boot_check.if_idx, 0, sizeof(s_boot_check.if_idx));
			if (snprintf(s_boot_check.if_idx.ifr_name, sizeof(s_boot_check.if_idx.ifr_name), "%s", s_boot_check.ifName) < 0)
			{
				r = enum_boot_check_retcode_ERR_snprintf_idx_ifr_name;
			}
			else if (ioctl(s_boot_check.socket_RAW_tx, SIOCGIFINDEX, &s_boot_check.if_idx) < 0)
			{
				r = enum_boot_check_retcode_ERR_ioctl_SIOCGIFINDEX;
			}
		}

		if (r == enum_boot_check_retcode_OK)
		{
			/* Get the MAC address of the interface to send on */
			memset(&s_boot_check.if_mac, 0, sizeof(s_boot_check.if_mac));

			if (snprintf(s_boot_check.if_mac.ifr_name, sizeof(s_boot_check.if_mac.ifr_name), "%s",  s_boot_check.ifName) < 0)
			{
				r = enum_boot_check_retcode_ERR_snprintf_mac_ifr_name;
			}
			else if (ioctl(s_boot_check.socket_RAW_tx, SIOCGIFHWADDR, &s_boot_check.if_mac) < 0)
			{
				r = enum_boot_check_retcode_ERR_ioctl_SIOCGIFHWADDR;
			}
		}

		if (r == enum_boot_check_retcode_OK)
		{
			/* Construct the Ethernet header */
			memset(s_boot_check.sendbuf, 0, sizeof(s_boot_check.sendbuf));
			{
				u_int8_t *pether_shost_dst = &s_boot_check.eh->ether_shost[0];
				u_int8_t *p_src = (uint8_t *)&s_boot_check.if_mac.ifr_hwaddr.sa_data[0];
				/* copy the bytes of the MAC Ethernet header */
				memcpy(pether_shost_dst, p_src, sizeof(s_boot_check.eh->ether_shost));
			}
			{
				u_int8_t *pether_dhost_dst = &s_boot_check.eh->ether_dhost[0];
				u_int8_t *p_src = (u_int8_t *)my_dest_MAC;
				memcpy(pether_dhost_dst, p_src, sizeof(s_boot_check.eh->ether_dhost));
			}
			/* Ethertype field */
			s_boot_check.eh->ether_type = htons(ETH_P_IP);
			s_boot_check.tx_len += sizeof(struct ether_header);
		}
		if (r == enum_boot_check_retcode_OK)
		{
			struct iphdr *iph = (struct iphdr *) (s_boot_check.sendbuf + sizeof(struct ether_header));
			/* IP Header */
			iph->ihl 		= 5;
			iph->version 	= 4;
			iph->tos 		= 16; 	// Low delay
			iph->id 		= htons(54321);
			iph->ttl 		= 255; 	// hops
			iph->protocol 	= 17; 	// UDP
			/* Source IP address, can be spoofed */
			//iph->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));
			iph->saddr = inet_addr("0.0.0.0");
			/* Destination IP address */
			iph->daddr = inet_addr("255.255.255.255");
			s_boot_check.tx_len += sizeof(struct iphdr);

			struct udphdr *udph = (struct udphdr *) (s_boot_check.sendbuf + sizeof(struct iphdr) + sizeof(struct ether_header));
			/* UDP Header */
			udph->source = htons(3118);
			udph->dest = htons(3119);
			udph->check = 0; // skip
			s_boot_check.tx_len += sizeof(struct udphdr);


			/* Packet data */
			type_ASACZ_boot_packet * p_ASACZ_boot_packet = (type_ASACZ_boot_packet*)&s_boot_check.sendbuf[s_boot_check.tx_len];
			memset(p_ASACZ_boot_packet, 0, sizeof(*p_ASACZ_boot_packet));
			// we need to fill just the MAC address field, the others are not used
			memcpy(&p_ASACZ_boot_packet->mac_address[0], &s_boot_check.eh->ether_shost[0], sizeof(p_ASACZ_boot_packet->mac_address));
			s_boot_check.tx_len += sizeof(*p_ASACZ_boot_packet);


			/* Length of UDP payload and header */
			udph->len = htons(s_boot_check.tx_len - sizeof(struct ether_header) - sizeof(struct iphdr));
			/* Length of IP payload and header */
			iph->tot_len = htons(s_boot_check.tx_len - sizeof(struct ether_header));
			/* Calculate IP checksum on completed header */
			iph->check = csum((unsigned short *)(s_boot_check.sendbuf+sizeof(struct ether_header)), sizeof(struct iphdr)/2);
		}

		if (r == enum_boot_check_retcode_OK)
		{
			struct sockaddr_ll *p_sa = &s_boot_check.socket_address;
			/* Index of the network device */
			p_sa->sll_ifindex = s_boot_check.if_idx.ifr_ifindex;
			/* Address length*/
			p_sa->sll_halen = ETH_ALEN;
			/* Destination MAC */
			u_int8_t *p_src = (u_int8_t *)my_dest_MAC;
			memcpy(&p_sa->sll_addr[0], p_src, sizeof(p_sa->sll_addr));
		}


		// mark the socket as NON blocking
		if (r == enum_boot_check_retcode_OK)
		{
			int flags = fcntl(s_boot_check.socket_RAW_tx, F_GETFL, 0);
			int status_fcntl = fcntl(s_boot_check.socket_RAW_tx, F_SETFL, flags | O_NONBLOCK);
			if (status_fcntl < 0)
			{
				r = enum_boot_check_retcode_ERR_unable_set_socket_tx_non_blocking;
			}
		}


		if (r == enum_boot_check_retcode_OK)
		{
			// opens the socket
			s_boot_check.socket_UDP_rx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (s_boot_check.socket_UDP_rx < 0)
			{
				r = enum_boot_check_retcode_ERR_unable_open_socket_rx;
			}
		}

		if (r == enum_boot_check_retcode_OK)
		// mark the socket as NON blocking
		{
			int flags = fcntl(s_boot_check.socket_UDP_rx, F_GETFL, 0);
			if (flags == -1)
			{
				r = enum_boot_check_retcode_ERR_unable_read_flags_socket_rx;
			}
			if (fcntl(s_boot_check.socket_UDP_rx, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				r = enum_boot_check_retcode_ERR_unable_write_flags_socket_rx;
			}

		}

		if (r == enum_boot_check_retcode_OK)
		// do the bind
		{
			memset(&s_boot_check.server_address, 0, sizeof(s_boot_check.server_address));
			struct sockaddr_in * p = &s_boot_check.server_address;
			p->sin_family = AF_INET;
			p->sin_addr.s_addr = INADDR_ANY;
			uint16_t port_number = 3118;
			p->sin_port = htons(port_number);
			if (bind(s_boot_check.socket_UDP_rx, (struct sockaddr *) p,sizeof(*p)) < 0)
			{
				r = enum_boot_check_retcode_ERR_unable_to_bind_socket_rx;
			}
		}

		if (r == enum_boot_check_retcode_OK)
		{
			run.boot_check_initialized_OK = 1;
			my_log(LOG_INFO,"%s: boot initialized OK", __func__);
		}
		else
		{
			my_log(LOG_WARNING,"%s: boot initialization gone wrong: %s", __func__, get_boot_check_error_code_message(r));
			if (s_boot_check.socket_UDP_rx >= 0)
			{
				close(s_boot_check.socket_UDP_rx);
				s_boot_check.socket_UDP_rx = -1;
			}
			if (s_boot_check.socket_RAW_tx >= 0)
			{
				close(s_boot_check.socket_RAW_tx);
				s_boot_check.socket_RAW_tx = -1;
			}

			// IMPORTANT: put a pause here else the loop stops immediately!
			usleep(def_pause_between_start_boot_check_ms * 1000);
		}

	}


	if (r == enum_boot_check_retcode_OK)
	{
		uint32_t ui_num_loop_try_boot_check = 0;
		unsigned int ui_message_received_OK = 0;
		unsigned int ui_configured_OK = 0;
#ifdef def_infinite_loop
#warning infinite loop!
		my_log(LOG_INFO,"%s: sending broadcast message loop starts (infinite loop)", __func__);
		while(!ui_configured_OK)
#else
		my_log(LOG_INFO,"%s: sending broadcast message loop starts (timeout set to %u ms)", __func__, def_timeout_check_boot_message_ms);
		while(ui_num_loop_try_boot_check < def_max_tries_boot_check && (r == enum_boot_check_retcode_OK) && !ui_message_received_OK && !ui_configured_OK)
#endif
		{
#ifdef def_infinite_loop
			usleep(1000000);
			my_log(LOG_INFO,"%s: sending broadcast message ", __func__);
#else
			my_log(LOG_INFO,"%s: sending broadcast message loop %u of %u ", __func__, ui_num_loop_try_boot_check, def_max_tries_boot_check);
#endif
			ui_num_loop_try_boot_check++;
			/* Send packet */
			if (sendto(s_boot_check.socket_RAW_tx, s_boot_check.sendbuf, s_boot_check.tx_len, 0, (struct sockaddr*)&s_boot_check.socket_address, sizeof(struct sockaddr_ll)) < 0)
			{
				r = enum_boot_check_retcode_ERR_unable_to_send;
				continue;
			}
			char rx_buffer[4096];
			unsigned int idx_loop_rx;
			my_log(LOG_INFO,"%swaiting reply loop starts (timeout set to %u ms)", __func__, def_timeout_rx_reply_boot_message_ms);
#ifdef def_infinite_loop
			ui_message_received_OK = 0;
#endif
			for (idx_loop_rx = 0; idx_loop_rx < def_nloop_wait_reply_boot_message_ms && !ui_message_received_OK; idx_loop_rx++)
			{
#ifndef def_infinite_loop
				//my_log(LOG_INFO,"%s: waiting rx loop %u of %u ", __func__, idx_loop_rx + 1, def_nloop_wait_reply_boot_message_ms);
#endif
				int recv_retcode = recv(s_boot_check.socket_UDP_rx, rx_buffer, sizeof(rx_buffer), 0);
				if (recv_retcode > 0)
				{
					ui_message_received_OK = 1;
					my_log(LOG_INFO,"%s: END OF WAIT! received %u bytes: ", __func__, (unsigned int)recv_retcode);
					type_ASACZ_boot_packet *p_rx_boot_packet = (type_ASACZ_boot_packet *)&rx_buffer[0];
					unsigned int error_command = 0;
					{
						switch (p_rx_boot_packet->ip_conf_type[0])
						{
							default:
							{
								my_log(LOG_ERR,"%s: unknown ip_conf_type: <%u> ", __func__, (unsigned int)p_rx_boot_packet->ip_conf_type[0]);
								break;
							}
							case enum_boot_pack_ip_conf_type_DHCP:
							{
								my_log(LOG_INFO,"+%s: setting DHCP mode", __func__);
								if (!error_command)
								{
									error_command = !is_OK_execute_command("uci set network.lan.proto='dhcp'");
								}

								if (!error_command)
								{
#ifdef do_uci_commit
									error_command = !is_OK_execute_command("uci commit network");
#endif
								}

								if (!error_command)
								{
									error_command = !is_OK_execute_command("/etc/init.d/network restart");
								}
								if (!error_command)
								{
									ui_configured_OK = 1;
								}

								my_log(LOG_INFO,"-%s: setting DHCP mode", __func__);
								break;
							}
							case enum_boot_pack_ip_conf_type_static:
							{
								my_log(LOG_INFO,"%s: setting static mode %u.%u.%u.%u", __func__,
										 (unsigned int)(p_rx_boot_packet->ip_address[0])
										,(unsigned int)(p_rx_boot_packet->ip_address[1])
										,(unsigned int)(p_rx_boot_packet->ip_address[2])
										,(unsigned int)(p_rx_boot_packet->ip_address[3])
								);

								if (!error_command)
								{
									error_command = !is_OK_execute_command("uci set network.lan.ipaddr=%u.%u.%u.%u",
											 (unsigned int)(p_rx_boot_packet->ip_address[0])
											,(unsigned int)(p_rx_boot_packet->ip_address[1])
											,(unsigned int)(p_rx_boot_packet->ip_address[2])
											,(unsigned int)(p_rx_boot_packet->ip_address[3])
											);
								}
								if (!error_command)
								{
									error_command = !is_OK_execute_command("uci set network.lan.proto='static'");
								}
								if (!error_command)
								{
									if (       (unsigned int)(p_rx_boot_packet->subnet_mask[0])
											|| (unsigned int)(p_rx_boot_packet->subnet_mask[1])
											|| (unsigned int)(p_rx_boot_packet->subnet_mask[2])
											|| (unsigned int)(p_rx_boot_packet->subnet_mask[3])
									)
									{
										error_command = !is_OK_execute_command("uci set network.lan.netmask='%u.%u.%u.%u'",
												 (unsigned int)(p_rx_boot_packet->subnet_mask[0])
												,(unsigned int)(p_rx_boot_packet->subnet_mask[1])
												,(unsigned int)(p_rx_boot_packet->subnet_mask[2])
												,(unsigned int)(p_rx_boot_packet->subnet_mask[3])
												);
									}
									else
									{
										my_log(LOG_INFO,"%s: no net mask set, skipping", __func__);
									}
								}
								if (!error_command)
								{
									if ((p_rx_boot_packet->gateway[0]) || (p_rx_boot_packet->gateway[1]) || (p_rx_boot_packet->gateway[2]) || (p_rx_boot_packet->gateway[3]) )
									{
										my_log(LOG_INFO,"%s: setting the gateway to %u.%u.%u.%u", __func__
												,(unsigned int )(p_rx_boot_packet->gateway[0])
												,(unsigned int )(p_rx_boot_packet->gateway[1])
												,(unsigned int )(p_rx_boot_packet->gateway[2])
												,(unsigned int )(p_rx_boot_packet->gateway[3])
										);
										error_command = !is_OK_execute_command("uci delete dhcp.lan.dhcp_option");
										if (!error_command)
										{
											error_command = !is_OK_execute_command("uci add_list dhcp.lan.dhcp_option='3,%u.%u.%u.%u'"
													,(unsigned int )(p_rx_boot_packet->gateway[0])
													,(unsigned int )(p_rx_boot_packet->gateway[1])
													,(unsigned int )(p_rx_boot_packet->gateway[2])
													,(unsigned int )(p_rx_boot_packet->gateway[3])
													);
										}
									}
									else
									{
										my_log(LOG_INFO,"%s: no gateway set", __func__);
									}
								}
								// DNS
								/*
								 *  * DNS
 *  uci delete dhcp.@dnsmasq[-1]
    uci add dhcp dnsmasq
    uci set dhcp.@dnsmasq[-1].server='8.8.8.8'
    uci add_list dhcp.@dnsmasq[-1].server='8.8.8.4'
								 *
								 */
								if (!error_command)
								{
									unsigned int dns_primary_set = (p_rx_boot_packet->dns_primary[0] || p_rx_boot_packet->dns_primary[1] || p_rx_boot_packet->dns_primary[2] || p_rx_boot_packet->dns_primary[3]);
									unsigned int dns_alternative_set = (p_rx_boot_packet->dns_alternative[0] || p_rx_boot_packet->dns_alternative[1] || p_rx_boot_packet->dns_alternative[2] || p_rx_boot_packet->dns_alternative[3]);
									// if DNS set...
									if (    dns_primary_set
										 || dns_alternative_set
									)
									{
										my_log(LOG_INFO,"%s: setting DNS server(s)", __func__);
										error_command =    !is_OK_execute_command("uci delete dhcp.@dnsmasq[-1]")
												        || !is_OK_execute_command("uci add dhcp dnsmasq");

										if (!error_command && dns_primary_set)
										{
											error_command = !is_OK_execute_command("uci set dhcp.@dnsmasq[-1].server='%u.%u.%u.%u'"
													,(unsigned int )(p_rx_boot_packet->dns_primary[0])
													,(unsigned int )(p_rx_boot_packet->dns_primary[1])
													,(unsigned int )(p_rx_boot_packet->dns_primary[2])
													,(unsigned int )(p_rx_boot_packet->dns_primary[3])
													);
										}
										if (!error_command && dns_alternative_set)
										{
											error_command = !is_OK_execute_command("uci set dhcp.@dnsmasq[-1].server='%u.%u.%u.%u'"
													,(unsigned int )(p_rx_boot_packet->dns_alternative[0])
													,(unsigned int )(p_rx_boot_packet->dns_alternative[1])
													,(unsigned int )(p_rx_boot_packet->dns_alternative[2])
													,(unsigned int )(p_rx_boot_packet->dns_alternative[3])
													);
										}
									}
									else
									{
										my_log(LOG_INFO,"%s: no DNS server set", __func__);

									}
								}

								if (!error_command)
								{
#ifdef do_uci_commit
									error_command = !is_OK_execute_command("uci commit network");
#endif
								}

								if (!error_command)
								{
									error_command = !is_OK_execute_command("/etc/init.d/network restart");
								}
								if (!error_command)
								{
									ui_configured_OK = 1;
								}

								my_log(LOG_INFO,"-%s: setting static mode", __func__);
								break;
							}
						}
					}
					{
						int n_to_print = recv_retcode;
						char *src = rx_buffer;
						while(n_to_print > 0)
						{
							char msg[128];
							char *pc = msg;
							int res_bytes = sizeof(msg);
							unsigned int idx;
							int n_print = 16;
							if (n_print > n_to_print)
							{
								n_print = n_to_print;
							}
							int done_bytes = snprintf(pc, res_bytes, "%s: %i HEX: ", __func__, n_print);
							for (idx = 0; idx < n_print; idx++)
							{
								if (done_bytes > 0 && res_bytes > 0)
								{
									pc += done_bytes;
									res_bytes -= done_bytes;
									done_bytes = snprintf(pc, res_bytes, "%02X ", ((uint32_t)src[idx])&0xff);
								}
								else
								{
									break;
								}
							}
							my_log(LOG_INFO,"%s: %s", __func__, msg);
							src += idx;
							n_to_print -= idx;
						}
					}
				}
				if (!ui_message_received_OK && !ui_configured_OK)
				{
					usleep(100000);
				}
				else
				{
					break;
				}
			}
		}
		if (ui_configured_OK)
		{
			my_log(LOG_INFO,"%s: network configured OK, waiting some seconds...", __func__);
			unsigned int i;
			for (i = 0; i < def_seconds_pause_network_reconfigured; i++)
			{
				usleep(1000 * 1000);
				my_log(LOG_INFO,"%s: %i seconds out of %i...", __func__, i + 1, def_seconds_pause_network_reconfigured);
			}
		}
	}
	const char *return_code_message = get_boot_check_error_code_message(r);
	if (r == enum_boot_check_retcode_OK)
	{
		my_log(LOG_INFO,"%s: OK, return code %s", __func__, return_code_message);
	}
	else
	{
		my_log(LOG_ERR,"%s: ERROR, return code %s", __func__, return_code_message);
	}
	my_log(LOG_INFO,"%s: - ends", __func__);
	if (s_boot_check.socket_RAW_tx >= 0)
	{
		close(s_boot_check.socket_RAW_tx);
		s_boot_check.socket_RAW_tx = -1;
	}
	if (s_boot_check.socket_UDP_rx >= 0)
	{
		close(s_boot_check.socket_UDP_rx);
		s_boot_check.socket_UDP_rx = -1;
	}
	return r;
}
