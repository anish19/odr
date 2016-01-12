/*DS and Func common b/w client and odr or server and odr*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include "unp.h"
#include "hw_addrs.h"
#include "list.h"

#define ETH_P_AA19 0x999
#define IF_NAME 16
#define ETH_PACK_SIZE 14+ (int)sizeof(struct odr_pack) 
//142	//(6+6+2) + (4+15+15 16+12 +64)
//#define SERVER_PORT 15960

#define TYPE_RREQ	0
#define TYPE_RREP	1
#define TYPE_DATA	2

struct rreq_hdr;
struct rrep_hdr;

//struct to hold interface info

struct if_info{
	struct sockaddr_ll* physical_addr;
	struct sockaddr* ip_addr;
	char if_name[IF_NAME];
	int ll_sockfd;
	struct list_head list;
};

struct rreq_hdr{
	int hop;
	int broadcast_id;
	int force_discovery;
	int rrep_sent;
};

struct rrep_hdr{
	int hop;
	int broadcast_id;
	int force_discovery;
};


struct odr_pack{
	int type;
	char dest_can_ip_str[15];
	char src_can_ip_str[15];
	//set pack type using eh->h_proto
	struct rreq_hdr rreq;
	struct rrep_hdr rrep;
	char payload[64];
	int src_dest;
};

char latest_rreq[11];	//has latest broadcast id for each node

//return self vm name
void print_self_name(){
	char hostname[5];
	gethostname(hostname, sizeof(hostname));
	printf("%s\n",hostname);
}

//gives string of cannonical ip addr for current vm
void get_can_ip(char* ip_str ){

	struct hostent *he;
	struct in_addr **addr_list;
	char hostname[5];
	int len;
	char *ret_str;

	//ip_str = (char*) malloc(15);
	gethostname(hostname, sizeof(hostname));
	he = gethostbyname(hostname);

	addr_list = (struct in_addr **)he->h_addr_list;
	ret_str = inet_ntoa(*addr_list[0]);
	
	strncpy(ip_str, ret_str, 15);
	ip_str[14] = '\0';	
	//len=strlen(ip_str);
	//printf("%s\n",ip_str)

}

int get_index_from_ip(char* ip_str){
	//idx for vm1 is 1 and vm 10 is 10
	struct sockaddr_in sa;
	char host[5];

	sa.sin_family=AF_INET;
	inet_pton(AF_INET, ip_str, &sa.sin_addr);
	getnameinfo(( struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL, 0, 0);
	
	if(host[3] == '0' && host[2] == '1'){
		return 10;
	}
	return (int) (host[2] - '0');

}

void print_hwaddr( char* hw_addr){
	int i;
	
	i = 0;
	while(i<6){
		printf("%.2x:", hw_addr[i] & 0xff);
		if(i == 5)
		   break;
		i++;
	}
	printf("\n");
}







