#include	"unp.h"
#include 	"hw_addrs.h"
#include	"ping.h"
#include    <netinet/in_systm.h>    /* required for ip.h */ 
#include    <netinet/in.h>
#include    <netinet/ip.h>
#include	"ip_var.h"
#include    <netinet/udp.h>
#include	"udp_var.h"
#include    <net/if.h>
#include    <netinet/if_ether.h>

#define 	TTL_OUT		1
#define 	IP_ID 		129
#define		IPPROTO_RT		129
#define 	P_NUM		1209
#define		MUL_P_NUM	1609


struct proto proto = { proc, sendPing, NULL, NULL, NULL, 0, IPPROTO_ICMP };

int 	datalen = 56;

int main(int argc, char **argv){

	int 				i, n, rt, mul_sendfd, mul_recvfd, usrlen, index, hasJoint, tmp, maxfdp1;
	const int 			on = 1;
	int 				isSend, isTitle; 	
	socklen_t			len;
	void				*buffer = (void *)calloc(1, MAXLINE);
	char				host_address[INET_ADDRSTRLEN];
	char				local_address[INET_ADDRSTRLEN], dest_address[INET_ADDRSTRLEN];
	char				src_address[INET_ADDRSTRLEN];
	char				mul_address[INET_ADDRSTRLEN] = "239.0.0.129"; //TTL = 1
	char				*mul_ip_ptr = buffer+28;
	uint16_t			*mul_pnum_ptr = buffer+44;
	uint16_t			*index_ptr = buffer+46;
	char				*ip_ptr = buffer+48;
	char				*h;
	char				host_name[100];
	struct addrinfo 	*ai;
	struct sockaddr_in 	*sin, local_in, dest_in, src_in;
	struct hwa_info     *hwa, *hwahead;
	struct sockaddr     *sa;
	struct sockaddr_in 	mul_sa;
	struct group_req 	req;
	struct udpiphdr 	*ui = (struct udpiphdr *) buffer;
	struct ip			*ip = (struct ip *) buffer;

	fd_set				rset;
	pid_t 				childpid;

	//initiate the parameter------------------------------------------------------
	hasJoint = 0;
	isTourOver = 0;
	isSend = 0;
	isTitle = 0;
	count = 0;

	//program begin---------------------------------------------------------------
	if(argc >= 2){
		printf("\n<<<<<Here is source node>>>>>\n\n");
	}else if(argc == 1){
		printf("\n<<<<<Here is passing node>>>>>\n\n");
	}else{
		err_quit("[ERROR]: usage: ./tour_module or ./tour_module <hostname>... ");
	}
	
	opterr = 0;
	*index_ptr = htons(1);
	*mul_pnum_ptr = htons(MUL_P_NUM);
	memcpy(mul_ip_ptr, mul_address, INET_ADDRSTRLEN);

	while(optind < argc){
		//stdout the ip address
		ai = host_serv(argv[optind], NULL, AF_INET, 0);
		memcpy(host_address, sock_ntop_host(ai->ai_addr, ai->ai_addrlen), INET_ADDRSTRLEN);
		freeaddrinfo(ai);
		printf("%s: %s\n", argv[optind++], host_address);

		//store in the buffer
		memcpy(ip_ptr, host_address, INET_ADDRSTRLEN);
		ip_ptr += INET_ADDRSTRLEN;
	}
	//back to begin pointer
	ip_ptr = buffer+48;

	//print out the ip list from buffer
	printf("The list index pointer: %d\n", ntohs(*index_ptr));
	i = 0;
	while(i < argc-1){
		printf("the ip[%d] list: %s\n", i, ip_ptr);
		ip_ptr += INET_ADDRSTRLEN;
		i++;
	}
	ip_ptr = buffer+48;

	//obtain host ip address------------------------------------------------------
	for(hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next){
        if(hwa->if_index == 2){
            if ( (sa = hwa->ip_addr) != NULL){
                strncpy(local_address, Sock_ntop_host(sa, sizeof(*sa)), INET_ADDRSTRLEN);
                break;
            }
        }
    }
    printf("The local IP address<eth0> is: %s\n", local_address);

    //*****build up the socket*****-------------------------------------------------
	//build the route traversal raw socket----------------------------------------------
	if((rt = socket(AF_INET, SOCK_RAW, IPPROTO_RT)) < 0){
		err_sys("[ERROR]: route traversal socket build error");
	}
	if(setsockopt(rt, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("[ERROR]: setsockopt error");
	}
	if(setsockopt(rt, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
		err_sys("[ERROR]: route traversal setsockopt<IP_HDRINCL> error");
	}

	//build the two UDP multicast socket--------------------------------------------
	if((mul_sendfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		err_sys("[ERROR]: multicast send socket build error");
	}
	if((mul_recvfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		err_sys("[ERROR]: multicast receive socket build error");
	}
	if(setsockopt(mul_recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("[ERROR]: setsockopt error");
	}
	//set up sockaddr struct of multicast addr
	bzero(&mul_sa, sizeof(mul_sa));
	mul_sa.sin_family = AF_INET;
	mul_sa.sin_port = htons(MUL_P_NUM);
	if(inet_pton(AF_INET, mul_address, &mul_sa.sin_addr) <= 0){
		err_sys("[ERROR]: inet_pton error...");
	}
	if(bind(mul_recvfd, (SA *)& mul_sa, sizeof(mul_sa)) < 0){
		err_sys("[ERROR]: bind error");
	}
	req.gr_interface = 0;
	memcpy(&req.gr_group, (SA *)& mul_sa, sizeof(mul_sa));
	if(argc >= 2){
		printf("\n[INFO]: this node is added to the multicast group <%s>\n", sock_ntop((SA *) &mul_sa, sizeof(mul_sa)));
		if(setsockopt(mul_recvfd, family_to_level(((SA *)& mul_sa)->sa_family), MCAST_JOIN_GROUP, &req, sizeof(req)) < 0){
			err_sys("[ERROR]: setsockopt error");
		}
	}

	//build up the ping receive raw socket------------------------------------------
	if((pg = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0){
		err_sys("[ERROR]: ping receive socket build error");
	}
	if(setsockopt(pg, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("[ERROR]: setsockopt error");
	}

	//build up the ping request raw socket------------------------------------------
	if((pg_req = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0){
		err_sys("[ERROR]: ping request socket build error");
	}
	if(setsockopt(pg_req, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("[ERROR]: setsockopt error");
	}

	//------------------------------------------------------------------------------

	usrlen = 20 + (argc-1) * INET_ADDRSTRLEN;

	//set up sockaddr struct of local addr
	bzero(&local_in, sizeof(local_in));
	local_in.sin_family = AF_INET;
	local_in.sin_port = htons(P_NUM);
	if(inet_pton(AF_INET, local_address, &local_in.sin_addr) <= 0){
		err_sys("[ERROR]: inet_pton error");
	}

	//when this node is a source node---------------------------------------------
	if(argc > 1){
		//set up sockaddr struct of dest addr
		bzero(&dest_in, sizeof(dest_in));
		dest_in.sin_family = AF_INET;
		dest_in.sin_port = htons(P_NUM);
		memcpy(dest_address, ip_ptr, INET_ADDRSTRLEN);
		if(inet_pton(AF_INET, dest_address, &dest_in.sin_addr) <= 0){
			err_sys("[ERROR]: inet_pton error...");
		}

		//send out the routing message
		if((n = rt_sendto(rt, buffer, usrlen, (SA *) &local_in, (SA *) &dest_in, sizeof(dest_in))) < 0){
			err_sys("[ERROR]: rt_send error");
		}else{
			printf("[INFO]: raw socket sends %d-bytes data\n", n);
		}
	}


	//use select to monitor all the sockets for rececving data------------------------
	while(1){
		//join the multicast group
		if(hasJoint == 1 && argc == 1){
			printf("\n[INFO]: this node is added to the multicast group <%s>\n", sock_ntop((SA *) &mul_sa, sizeof(mul_sa)));
			if(setsockopt(mul_recvfd, family_to_level(((SA *)& mul_sa)->sa_family), MCAST_JOIN_GROUP, &req, sizeof(req)) < 0){
				err_sys("[ERROR]: setsockopt error");
			}
			hasJoint = 2;
		}

		if(dest_address[0] == '\0' && count == 20){
			printf("\n----------------------------------------\n\
[INFO]: *****This tour is finished!*****\n\
----------------------------------------\n\n");
			if(send_eot(mul_sendfd, (SA *) &mul_sa, sizeof(mul_sa)) < 0){
				err_sys("[ERROR]: send_eot error");
			}
			count++;
		}

		FD_ZERO(&rset);
		//caculate the max fd
		tmp = rt;
		tmp = max(mul_recvfd, tmp);
		tmp = max(pg, tmp);
		maxfdp1 = tmp + 1;
		//add the socket to read set
		FD_SET(rt, &rset);
		FD_SET(mul_recvfd, &rset);
		FD_SET(pg, &rset);
		if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0){
			if(errno == EINTR)
				continue;
			else
				err_sys("[ERROR]: select error...");
		}

		Signal(SIGALRM, sig_alrm);

		//monitor the routing traversal socket
		if(FD_ISSET(rt, &rset)){
			//block in recvfrom to wait for touring packet
			bzero(buffer, sizeof(buffer));
			len = sizeof(src_in);
			if((n = rt_recvfrom(rt, buffer, (SA *) &src_in, len, &index, dest_address)) < 0){
				err_sys("[ERROR]: rt_recvfrom error");
			}else if(n > 0){
				usrlen = n - 28; //minus the udp ip header length

				//begin pinging-------------------------------------------
				if(isSend == 0){

					//get the preceding IP address
					index = ntohs(*index_ptr);
					host = inet_ntoa(*(struct in_addr *) &ui->ui_src.s_addr);
					addr2name(host_name, host);
					pid = getpid() & 0xffff; 

					ai = Host_serv (host_name, NULL, 0, 0);

					h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);

					/* initialize  according to protocol */
					pr = &proto;
					pr->sasend = ai->ai_addr;
					pr->sarecv = Calloc (1, ai->ai_addrlen);
					pr->salen = ai->ai_addrlen;

					sig_alrm(SIGALRM);
					isSend = 1;

				}
				//----------------------------------------------------------

				//print out the receive info
				//printf("[INFO]: rt socket receives %d-bytes data\n", n);
				//printf("The received mul_address: %s\n", mul_ip_ptr);
				//printf("The received mul port number: %d\n", ntohs(*mul_pnum_ptr));
				//printf("The received index: %d\n", index);
				//printf("The received dest_address: %s\n\n", dest_address);
				if(dest_address[0] == '\0'){
					continue;
				}
			}

			//relay the touring packet
			bzero(&dest_in, sizeof(dest_in));
			dest_in.sin_family = AF_INET;
			dest_in.sin_port = htons(P_NUM);
			if(inet_pton(AF_INET, dest_address, &dest_in.sin_addr) <= 0){
				err_sys("[ERROR]: inet_pton error...");
			}
			if((n = rt_sendto(rt, buffer, usrlen, (SA *) &local_in, (SA *) &dest_in, sizeof(dest_in))) < 0){
				err_sys("[ERROR]: rt_send error");
			}else{
				printf("[INFO]: rt socket sends %d-bytes data\n", n);
			}

			hasJoint++;
		}

		//monitor the UDP multicast receive socket
		if(FD_ISSET(mul_recvfd, &rset)){				
			if((n = recv_mul(mul_recvfd, sizeof(mul_sa))) < 0){
				err_sys("[ERROR]: recv_mul error");
			}
			//when receive the End Of Tour message
			if(n > 85){
				isTourOver = 1;
				if(send_mul(mul_sendfd, (SA *) &mul_sa, sizeof(mul_sa)) < 0){
					err_sys("[ERROR]: send_eot error");
				}
			}
		}

		if(FD_ISSET(pg, &rset)){
			if(isSend == 0){
				continue;
			}

			if(isTitle == 0){
				printf ("\nPING %s (%s): %d data bytes\n", 
							ai->ai_canonname ? ai->ai_canonname : h, h, datalen);
				isTitle = 1;
			}

			recvPing();	
		}
	}

	return 0;
}







