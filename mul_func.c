#include    "unp.h"
#include    <sys/utsname.h>
#include    <net/if.h>

int send_eot(int sendfd, SA *sadest, socklen_t salen){
	int 			n;
	char    		line[MAXLINE];      /* hostname and process ID */
	struct utsname 	myname;

	if (uname(&myname) < 0){
		err_sys("[ERROR]: uname error");
	}
	snprintf(line, sizeof(line), "<<<<< This is node %s. Tour has ended. Group members please identify yourselves. >>>>>", myname.nodename);

	if((n = sendto(sendfd, line, strlen(line), 0, sadest, salen)) < 0){
		return -1;
	}else{
		printf("\n--------------------------------------------------------------------------\n\
Node %s.  Sending: %s\n\
--------------------------------------------------------------------------\n\n", myname.nodename, line);
	}
	
	return n;
}

int send_mul(int sendfd, SA *sadest, socklen_t salen){
	int 			n;
	char    		line[MAXLINE];      /* hostname and process ID */
	struct utsname 	myname;

	if (uname(&myname) < 0){
		err_sys("[ERROR]: uname error");
	}
	snprintf(line, sizeof(line), "<<<<< Node %s. I am a member of the group. >>>>>", myname.nodename);

	if((n = sendto(sendfd, line, strlen(line), 0, sadest, salen)) < 0){
		return -1;
	}else{
		printf("\n--------------------------------------------------------------------------\n\
Node %s.  Sending: %s\n\
--------------------------------------------------------------------------\n\n", myname.nodename, line);
	}
	
	return n;
}

int recv_mul(int recvfd, socklen_t salen){
	int     		n;
	char    		line[MAXLINE + 1];
	socklen_t 		len;
	struct sockaddr *safrom;
	struct utsname	myname;

	safrom = malloc(salen);
	if (uname(&myname) < 0){
		err_sys("[ERROR]: uname error");
	}
	
	len = salen;
	if((n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0){
		return -1;
	}

	line[n] = 0;      
	printf("\n--------------------------------------------------------------------------\n\
Node %s. Received: %s\n\
--------------------------------------------------------------------------\n\n", myname.nodename, line);

	return n;
}