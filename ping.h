#include    "unp.h"
#include    <netinet/in_systm.h>
#include    <netinet/ip.h>
#include    <netinet/ip_icmp.h>

#define BUFSIZE     1500

/* globals */
char    sendbuf[BUFSIZE];

int     datalen;                /* #bytes of data following ICMP header */
char   	*host;
char	src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
int     nsent;                  /* add 1 for each sendto() */
pid_t   pid;                    /* our PID */
int 	pg;						/* pinging receive socket */
int     pg_req;                	/* pinging pf_packet request socket */

int 	isTourOver;
int  	count;

/* function prototypes */
void    proc(char *, ssize_t, struct msghdr *, struct timeval *);
void    sendPing(void);
void    recvPing(void);
void 	sig_alrm(int);
void    tv_sub(struct timeval *, struct timeval *);

struct proto {
	void    (*fproc) (char *, ssize_t, struct msghdr *, struct timeval *);
	void    (*fsend) (void);
	void    (*finit) (void);
	struct sockaddr *sasend;    /* sockaddr{} for send, from getaddrinfo */
	struct sockaddr *sarecv;    /* sockaddr{} for receiving */
	socklen_t salen;            /* length of sockaddr {}s */
	int     icmpproto;          /* IPPROTO_xxx value for ICMP */
} *pr;
