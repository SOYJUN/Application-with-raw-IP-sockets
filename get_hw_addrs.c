#include <errno.h>		/* error numbers */
#include <sys/ioctl.h>          /* ioctls */
#include <net/if.h>             /* generic interface structures */

#include "hw_addrs.h"


struct hwa_info *
get_hw_addrs()
{
	struct hwa_info	*hwa, *hwahead, **hwapnext;
	int		sockfd, len, lastlen, alias, nInterfaces, i;
	char		*ptr, *buf, lastname[IF_NAME], *cptr;
	struct ifconf	ifc;
	struct ifreq	*ifr, *item, ifrcopy;
	struct sockaddr	*sinptr;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char*) Malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}

	hwahead = NULL;
	hwapnext = &hwahead;
	lastname[0] = 0;
    
	ifr = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)  {
		item = &ifr[i];
 		alias = 0; 
		hwa = (struct hwa_info *) Calloc(1, sizeof(struct hwa_info));
		memcpy(hwa->if_name, item->ifr_name, IF_NAME);		/* interface name */
		hwa->if_name[IF_NAME-1] = '\0';
				/* start to check if alias address */
		if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon will null */
		if (strncmp(lastname, item->ifr_name, IF_NAME) == 0) {
			alias = IP_ALIAS;
		}
		memcpy(lastname, item->ifr_name, IF_NAME);
		ifrcopy = *item;
		*hwapnext = hwa;		/* prev points to this new one */
		hwapnext = &hwa->hwa_next;	/* pointer to next one goes here */

		hwa->ip_alias = alias;		/* alias IP address flag: 0 if no; 1 if yes */
                sinptr = &item->ifr_addr;
		hwa->ip_addr = (struct sockaddr *) Calloc(1, sizeof(struct sockaddr));
	        memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));	/* IP address */
		if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0)
                          perror("SIOCGIFHWADDR");	/* get hw address */
		memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IF_HADDR);
		if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0)
                          perror("SIOCGIFINDEX");	/* get interface index */
		memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
	}
	free(buf);
	return(hwahead);	/* pointer to first structure in linked list */
}

void
free_hwa_info(struct hwa_info *hwahead)
{
	struct hwa_info	*hwa, *hwanext;

	for (hwa = hwahead; hwa != NULL; hwa = hwanext) {
		free(hwa->ip_addr);
		hwanext = hwa->hwa_next;	/* can't fetch hwa_next after free() */
		free(hwa);			/* the hwa_info{} itself */
	}
}
/* end free_hwa_info */

struct hwa_info *
Get_hw_addrs()
{
	struct hwa_info	*hwa;

	if ( (hwa = get_hw_addrs()) == NULL)
		err_quit("get_hw_addrs error");
	return(hwa);
}

