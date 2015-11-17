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
#define SRV_UN_PATH "/tmp/kool_srv_path-XXXXXX"
#define IF_NAME 16
#define ETH_PACK_SIZE 110	//(6+6+2) + (4+6+6+8+8+64)

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
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	//set pack type using eh->h_proto
	struct rreq_hdr* rreq;
	struct rrep_hdr* rrep;
	char payload[64];
};


