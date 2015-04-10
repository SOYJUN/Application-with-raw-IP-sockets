#include     "ping.h"
#include 	 "hw_addrs.h"	
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_ICMP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define ICMP_HDRLEN 8  // ICMP header length for echo request, excludes data

void sendPing(){
	int     			i, len, sd, ip_flags[4];
	char				interface[40], src_mac[6], dest_mac[6];
	char				src_ipaddr[INET_ADDRSTRLEN], dest_ipaddr[INET_ADDRSTRLEN];
	struct ifreq 		ifr;
	struct sockaddr_ll	device;
	struct hwa_info     *hwa, *hwahead;
	struct sockaddr     *sa;

	struct icmp 		*icmp;
	struct ip 			*iphdr;
	unsigned char       *etherhead = sendbuf;
    struct ethhdr       *eh;

    // Interface to send packet through.
  	strcpy (interface, "eth0");

  	// Submit request for a socket descriptor to look up interface.
  	if ((sd = socket(PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
   		err_sys("[ERROR]: PF_PACKET socket build error");
  	}

  	// Use ioctl() to look up interface name and get its MAC address.
  	memset (&ifr, 0, sizeof (ifr));
  	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
  	if (ioctl (sd, SIOCGIFHWADDR, &ifr) < 0) {
  		err_sys("[ERROR]: ioctl() failed to get source MAC address");
    }
  	close (sd);

  	// Copy source MAC address.
 	memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6);

 	// Report source MAC address to stdout.
	printf ("MAC address for interface %s is ", interface);
	for (i=0; i<5; i++) {
		printf ("%02x:", src_mac[i]);
	}
	printf ("%02x\n", src_mac[5]);

	bzero(&device, sizeof(device));
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
    	err_sys("[ERROR]: if_nametoindex() failed to obtain interface index ");
  	}
  	printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);

  	// Set destination MAC address: you need to fill these out
  	//local mac 00:0c:29:da:5f:bd
	dest_mac[0] = 0x00;
	dest_mac[1] = 0x0c;
	dest_mac[2] = 0x29;
	dest_mac[3] = 0xda;
	dest_mac[4] = 0x5f;
	dest_mac[5] = 0xbd;

	// Source IPv4 address:
	//obtain host ip address------------------------------------------------------
	for(hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next){
        if(hwa->if_index == 2){
            if ( (sa = hwa->ip_addr) != NULL){
                strncpy(src_ipaddr, Sock_ntop_host(sa, sizeof(*sa)), INET_ADDRSTRLEN);
                break;
            }
        }
    }
    printf("The source IP address<eth0> is: %s\n", src_ipaddr);

	// Destination URL or IPv4 address: you need to fill this out
	strncpy (dest_ipaddr, "192.168.142.20", INET_ADDRSTRLEN);
	printf("The destination IP address<eth0> is: %s\n", dest_ipaddr);

	// Fill out sockaddr_ll.
	device.sll_family = AF_PACKET;
	memcpy (device.sll_addr, src_mac, 6);
	device.sll_halen = htons (ETH_ALEN);


    //ethernet header--------------------------------------------------
    eh = (struct ethhdr *)etherhead;
    //set frame header
    memcpy((void *)sendbuf, (void *)dest_mac, ETH_ALEN);
    memcpy((void *)(sendbuf + ETH_ALEN), (void *)src_mac, ETH_ALEN);
    eh->h_proto = ETH_P_IP;

    //ip header--------------------------------------------------------
    iphdr = (struct ip *)(sendbuf + 14);
	iphdr->ip_hl = IP4_HDRLEN / sizeof (uint32_t);
	iphdr->ip_v = 4;
	iphdr->ip_tos = 0;
	iphdr->ip_len = htons (IP4_HDRLEN + ICMP_HDRLEN + datalen);
	iphdr->ip_id = htons (0);
	ip_flags[0] = 0;
	ip_flags[1] = 0;
	ip_flags[2] = 0;
	ip_flags[3] = 0;
	iphdr->ip_off = htons ((ip_flags[0] << 15) + (ip_flags[1] << 14)
	                  + (ip_flags[2] << 13) +  ip_flags[3]);
	iphdr->ip_ttl = 255;
	iphdr->ip_p = IPPROTO_ICMP;

	// Source IPv4 address (32 bits)
	if ((inet_pton (AF_INET, src_ipaddr, &(iphdr->ip_src))) != 1) {
		err_sys("[ERROR]: inet_pton() failed");
	}

	// Destination IPv4 address (32 bits)
	if ((inet_pton (AF_INET, dest_ipaddr, &(iphdr->ip_dst))) != 1) {
		err_sys("[ERROR]: inet_pton() failed");
	}

	iphdr->ip_sum = 0;
  	iphdr->ip_sum = in_cksum((uint16_t *) &iphdr, IP4_HDRLEN);


    //icmp header-------------------------------------------------------
	icmp = (struct icmp *) (sendbuf + 34);
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = htons(pid);
	icmp->icmp_seq = htons(nsent++);
	memset (icmp->icmp_data, 0xa5, datalen); // fill with pattern 
	Gettimeofday ((struct timeval *) icmp->icmp_data, NULL);

	len = 14 + 20 + 8 + datalen;           // checksum ICMP header and data 
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);

	if(sendto (pg_req, sendbuf, len, 0, (struct sockaddr *) &device, sizeof (device)) < 0){
		err_sys("[ERROR]: ping request send error");
	}

}