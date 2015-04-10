#include "unp.h"

int addr2name(char * hostname, char * hostaddr)
{
	struct in_addr		inaddr;
	struct hostent		*hptr;
	char				**pptr, *tempcharp;				

	// If the input ip address is a valid IPv6 address, exit program.
	if (inet_pton(AF_INET6, hostaddr, &(inaddr.s_addr))==1){
		printf("[ERROR]: addr2name: The IPv6 address is not supported.\n"
			"\tPlease input a server domain name or an IPv4 address.\n");
		return -1;
	}

	// Use inet_pton to judge whether this is a valid IPv4 address.
	if (inet_pton(AF_INET, hostaddr, &(inaddr.s_addr))<1){
		printf("[ERROR]: addr2name: Not a valid IPv4 address.\n");
		return -1;
	}

	hptr = gethostbyaddr((const char *)&(inaddr.s_addr), (socklen_t) 4, AF_INET);
	// If the IP address has no Domain record, give warning and process hostent struct.
	if (hptr==NULL){
		tempcharp = (char *)&(inaddr.s_addr);
		hptr = &((struct hostent) {.h_name = NULL, .h_addr_list = &tempcharp});
	}
	
	strcpy(hostname, hptr->h_name);
	return 1;
}
