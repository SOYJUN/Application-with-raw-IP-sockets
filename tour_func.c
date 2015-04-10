#include	"unp.h"
#include    <netinet/in_systm.h>    /* required for ip.h */ 
#include    <netinet/in.h>
#include    <netinet/ip.h>
#include	"ip_var.h"
#include    <netinet/udp.h>
#include	"udp_var.h"
#include    <net/if.h>
#include    <netinet/if_ether.h>
#include 	<time.h>

#define 	TTL_OUT		1
#define 	IP_ID 		129
#define		IPPROTO_RT		129

uint16_t in_cksum (uint16_t * addr, int len){
	int     nleft = len;
	uint32_t sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		* (unsigned char *) (&answer) = * (unsigned char *) w;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16);     /* add carry */
	answer = ~sum;     /* truncate to 16 bits */
	return (answer);
}

int rt_sendto(int sock_rt, void *buf, int usrlen, SA* local, SA* dest, socklen_t destlen){

	int 				n, index;
	socklen_t			locallen = destlen;
	struct udpiphdr 	*ui = (struct udpiphdr *) buf;
	struct ip			*ip = (struct ip *) buf;
	uint16_t			*index_ptr = buf+46;
	char				*ip_ptr = buf+48;

	char				ipaddr[INET_ADDRSTRLEN];

	//fill in and checksum udp header
	bzero(ui, sizeof(ui));
	//add 8 to usrlen for pseudoheader length
	ui->ui_len = htons((uint16_t)(sizeof(struct udphdr) + usrlen));
	//then add 28 for ip datagram length
	usrlen += sizeof(struct udpiphdr);

	ui->ui_pr = IPPROTO_RT;
	ui->ui_src.s_addr = ((struct sockaddr_in *) local)->sin_addr.s_addr;
	ui->ui_dst.s_addr = ((struct sockaddr_in *) dest)->sin_addr.s_addr;
	ui->ui_sport = ((struct sockaddr_in *) local)->sin_port;
	ui->ui_dport = ((struct sockaddr_in *) dest)->sin_port;
	ui->ui_ulen = ui->ui_len;

#if 1         
	if((ui->ui_sum = in_cksum((u_int16_t *) ui, usrlen)) == 0){
		ui->ui_sum = 0xffff;
	}
#else
	ui->ui_sum = ui->ui_len;
#endif

	/* fill in rest of IP header; */
	/* ip_output() calcuates & stores IP header checksum */
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = 0;
#if defined(linux) || defined(__OpenBSD__)
    ip->ip_len = htons(usrlen);    /* network byte order */
#else
	ip->ip_len = usrlen;       /* host byte order */
#endif
	ip->ip_id = htons(IP_ID);            
	ip->ip_off = 0;             /* frag offset, MF and DF flags */
	ip->ip_ttl = TTL_OUT;

	//increase the index pointer
	index = ntohs(*index_ptr);
	index++;
	*index_ptr = htons(index);

	if((n = sendto(sock_rt, buf, usrlen, 0 ,dest, destlen)) < 0){
		return -1;
	}else{
		printf("[INFO]: message send out from %s\n", sock_ntop(local, locallen));
	}
	return n;
}

int rt_recvfrom(int sock_rt, void *buf, SA* src, socklen_t srclen, int* index, char* dest_address){

	int 				n;
	socklen_t			len;
	struct udpiphdr 	*ui = (struct udpiphdr *) buf;
	struct ip			*ip = (struct ip *) buf;
	uint16_t			*index_ptr = buf+46;
	char				*ip_ptr = buf+48;
	char 				t_msg[100];
	char				src_address[INET_ADDRSTRLEN];
	char				src_name[100];
	time_t              ticks;

	len = srclen;
	if((n = recvfrom(sock_rt, buf, MAXLINE, 0, src, &len)) < 0){
		return -1;
	}else{
		//judge the IP identification number
		if(ip->ip_id != htons(IP_ID)){
			return 0;
		}

		*index = ntohs(*index_ptr);
        //printf("\n\n[INFO]: list index pointer: %d\n", *index);
        memcpy(dest_address, ip_ptr + (*index - 1)*INET_ADDRSTRLEN, INET_ADDRSTRLEN);
		//printf("[INFO]: address list: %s\n", dest_address);

		ticks = time(NULL);
        snprintf(t_msg, sizeof(t_msg), "%.24s", ctime(&ticks));
        memcpy(src_address, sock_ntop_host(src, sizeof(struct sockaddr_in)), INET_ADDRSTRLEN);
        //addr2name(src_name, src_address);
        printf("\n--------------------------------------------------------------------------\n\
<%s>	received source routing packet from <%s>\n\
--------------------------------------------------------------------------\n\n", t_msg, src_address);
	}
	return n;
}
