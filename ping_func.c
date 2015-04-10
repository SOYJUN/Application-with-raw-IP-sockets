#include     "ping.h"

void sendPing(){
	int     	len;
	struct icmp *icmp;

	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = nsent++;
	memset (icmp->icmp_data, 0xa5, datalen); // fill with pattern 
	Gettimeofday ((struct timeval *) icmp->icmp_data, NULL);

	len = 8 + datalen;           // checksum ICMP header and data 
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum ((u_short *) icmp, len);

	Sendto (pg, sendbuf, len, 0, pr->sasend, pr->salen);

}


void proc(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv){
	int     hlenl, icmplen;
	double  rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (struct ip *) ptr;      /* start of IP header */
	hlenl = ip->ip_hl << 2;      /* length of IP header */
	if (ip->ip_p != IPPROTO_ICMP)
		return;                  /* not ICMP */

	icmp = (struct icmp *) (ptr + hlenl);   /* start of ICMP header */
	if ( (icmplen = len - hlenl) < 8)
		return;                  /* malformed packet */

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)
			return;                /* not a response to our ECHO_REQUEST */
		if (icmplen < 16)
			return;                /* not enough data to use */

		tvsend = (struct  timeval  *) icmp->icmp_data;
		tv_sub (tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		printf ("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
	                 icmplen, Sock_ntop_host (pr->sarecv, pr->salen),
	                icmp->icmp_seq, ip->ip_ttl, rtt);

	}
}

void recvPing(){
	int    		 	size;
	char   		 	recvbuf[BUFSIZE];
	char    		controlbuf[BUFSIZE];
	struct msghdr 	msg;
	struct iovec 	iov;
	ssize_t 		n;
	struct timeval 	tval;

	size = 60 * 1024;          
	setsockopt (pg, SOL_SOCKET, SO_RCVBUF, &size, sizeof (size));

	iov.iov_base = recvbuf;
	iov.iov_len = sizeof (recvbuf);
	msg.msg_name = pr->sarecv;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = controlbuf;

	msg.msg_namelen = pr->salen;
	msg.msg_controllen = sizeof (controlbuf);

	n = recvmsg (pg, &msg, 0);

	if (n < 0) {
		if (errno == EINTR){
		}else{
			err_sys("[ERROR]: recvmsg error");
		}
	}
	Gettimeofday (&tval, NULL);
	proc(recvbuf, n, &msg, &tval);
	count++;
}

void tv_sub (struct timeval *out, struct timeval *in){
	if ((out->tv_usec -= in->tv_usec) < 0) {     /* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

void sig_alrm (int signo){
	if(isTourOver == 0){
		sendPing();
		alarm(1);
	}
	return;
}
