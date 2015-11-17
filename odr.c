#include "odr.h"

int ll_sockfd;
char ifname[4];

int broadcast_ll_msg( struct if_info* if_info_list){
	
	int ret = 0;
	//broadcast address
	struct sockaddr_ll* bc_addr;	
	const unsigned char broadcast_addr[] = {0xff,0xff,0xff,0xff,0xff,0xff};
	struct if_info* tmp;
	char msg_str[64];
	int i;
	struct odr_pack* packet;
	struct ethhdr* eh;
	void *buffer;
	char *tmch;

	packet = (struct odr_pack*)malloc(sizeof(struct odr_pack));
	buffer = malloc(ETH_PACK_SIZE);
//	printf("sizeof int while allocation is %ld\n", sizeof(i));
//	printf("sizeof rrep_hdr* is %ld\n", sizeof(struct rreq_hdr*));
	eh =  buffer;
	strncpy(msg_str, "this is broadcast\n", 18);

	printf("in broadcast_ll_msg\n");
	bc_addr = (struct sockaddr_ll*) malloc(sizeof(struct sockaddr_ll));
	list_for_each_entry_reverse( tmp, &(if_info_list->list), list){
		
		//setting send address
		bc_addr->sll_family = AF_PACKET;
		bc_addr->sll_ifindex = tmp->physical_addr->sll_ifindex;
		bc_addr->sll_halen = IF_HADDR;
		bc_addr->sll_protocol = htons(ETH_P_AA19);
		memcpy(bc_addr->sll_addr, broadcast_addr, IF_HADDR);
				
		bc_addr->sll_addr[6] = 0x00;	
		bc_addr->sll_addr[7] = 0x00;	
/*		printf("sending broadcast to %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
			bc_addr->sll_addr[0],	
			bc_addr->sll_addr[1],	
			bc_addr->sll_addr[2],	
			bc_addr->sll_addr[3],	
			bc_addr->sll_addr[4],	
			bc_addr->sll_addr[5])	;
	
		printf("	Interface name is: %s\n", tmp->if_name);
		printf("	Hardware address is: ");
		i = 0;
		while(i<6){
			//printf("i:%d\n", i);
			printf("%.2x",tmp->physical_addr->sll_addr[i] & 0xff);
			if(i!=5) printf(":");
			i++;
		}
		printf("\n");
		
		printf("	Interface index is: %d\n", tmp->physical_addr->sll_ifindex);
		printf("	Interface IP addr is: %s\n", Sock_ntop(tmp->ip_addr, sizeof(SA*)) );
		printf("	Interface bound to socket: %d\n", tmp->ll_sockfd);
		printf("\n");
*/
		/*ARP hardware identifier is ethernet*/
    	//bc_addr->sll_hatype   = ARPHRD_ETHER;

	    /*target is another host*/
		//bc_addr->sll_pkttype  = PACKET_BROADCAST;

		//filling odr_pack
		packet->type = TYPE_RREQ;
		memcpy(packet->dest_mac, broadcast_addr, 6);
		memcpy(packet->src_mac, tmp->physical_addr->sll_addr, 6);
		
		packet->rreq = (struct rreq_hdr*) malloc(sizeof(struct rreq_hdr));
		packet->rreq->hop = 0;
		packet->rreq->broadcast_id = 0;
		packet->rreq->force_discovery = 0;
		packet->rreq->rrep_sent = 0;
		memcpy(packet->payload, msg_str, 18);
		
		//filling ethernet layer packet
		memcpy(buffer, broadcast_addr, 6);
		memcpy(buffer+6, tmp->physical_addr->sll_addr, 6);
		memcpy(buffer+14, (void*)packet, sizeof(struct odr_pack));
		eh->h_proto = htons(ETH_P_AA19);
	
//		printf("size of buffer is %ld\n", sizeof(buffer));
		printf("msg buffer is %s\n",(char*)buffer+14);
		//ret = sendto(ll_sockfd, (char*)packet, 1018, 0, (SA*)bc_addr, sizeof(struct sockaddr_ll) );
		ret = sendto(ll_sockfd, buffer, ETH_PACK_SIZE, 0, (SA*)bc_addr, sizeof(struct sockaddr_ll) );
		printf("send ret is %d\n", ret);
	}
	if(ret>=0)
		return 1;
	else 
		return -1;
	
}

int fill_if_info_list(struct if_info* if_info_list ){
	
	struct if_info *tmp;
	struct hwa_info *hwahead, *hwa;

	hwahead = (struct hwa_info*) malloc(sizeof(struct hwa_info));
	hwa = (struct hwa_info*) malloc(sizeof(struct hwa_info));
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next ){
/*	
		printf("Interface name is: %s\n", hwa->if_name);
		printf("Hardware address is: %s\n", hwa->if_haddr);
		printf("Interface index is: %d\n", hwa->if_index);
		printf("Interface ip_alias is: %d\n", hwa->ip_alias);
		printf("Interface IP addr is: %s\n", Sock_ntop(hwa->ip_addr, sizeof(SA*)) );
*/
		//entry in if_info_list if not loopback or eth0
		if(strncmp(hwa->if_name, "lo", 2)!=0 && strncmp(hwa->if_name, "eth0", 4) != 0){
			tmp = (struct if_info*)malloc(sizeof(struct if_info));
			
			tmp->physical_addr = (struct sockaddr_ll*)malloc(sizeof(struct sockaddr_ll));
			memcpy(tmp->physical_addr->sll_addr, hwa->if_haddr, IF_HADDR);
			tmp->physical_addr->sll_ifindex = hwa->if_index;
			
			tmp->ip_addr = (SA*)malloc(sizeof(SA));
			memcpy(tmp->ip_addr, hwa->ip_addr, sizeof(SA));
			
			strncpy(tmp->if_name, hwa->if_name, strlen(hwa->if_name));
			
			tmp->ll_sockfd = 0;
			
			list_add(&(tmp->list), &(if_info_list->list));
			strncpy(ifname, hwa->if_name, 4);
		}
	}

	return 1;
}


int bind_if_to_socket( struct if_info* if_info_list, int* maxfd){
	
	//ll raw socket to send
	//int ll_sockfd;
	struct if_info *tmp;
	int i;

	printf("Interface info -\n");
	list_for_each_entry_reverse( tmp, &(if_info_list->list), list){
		//init ll Raw Socket
		//ll_sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_KOOL));
		//printf("Socket bound to %s\n", Sock_ntop(tmp->ip_addr, sizeof(SA*)));
		tmp->physical_addr->sll_family = AF_PACKET;
		tmp->physical_addr->sll_protocol = htons(ETH_P_AA19);
		tmp->physical_addr->sll_halen = IF_HADDR;
		tmp->ll_sockfd = ll_sockfd;
			
		printf("	Interface name is: %s\n", tmp->if_name);
		printf("	Hardware address is: ");
		i = 0;
		while(i<6){
			//printf("i:%d\n", i);
			printf("%.2x",tmp->physical_addr->sll_addr[i] & 0xff);
			if(i!=5) printf(":");
			i++;
		}
		printf("\n");
		
		printf("	Interface index is: %d\n", tmp->physical_addr->sll_ifindex);
		printf("	Interface IP addr is: %s\n", Sock_ntop(tmp->ip_addr, sizeof(SA*)) );
		printf("	Interface bound to socket: %d\n", tmp->ll_sockfd);
		printf("\n");

	//	Bind(tmp->ll_sockfd, (SA*)tmp->physical_addr , sizeof(SA) );
	//	if(ll_sockfd > *maxfd){
	//		*maxfd = ll_sockfd;	
	//	}
	}

	return 1;
}

int main(){

	int ret = 0;
	int un_sockfd;
	
	//for temp recv
	struct sockaddr_ll recv_addr;
	void *buffer;
	int temp_len;

	//unix socket
	struct sockaddr_un un_server_addr;	//link layer server
	struct sockaddr_un al_client_addr;	//application layer client
	int al_client_addr_size;

	struct hwa_info *hwa, *hwahead;

	//ll raw socket all
	//int ll_sockfd;
	struct sockaddr_ll recvfrom_addr;
	struct sockaddr_ll sendto_addr;
	struct sockaddr_ll self_addr;
	struct if_info if_info_list;
	struct if_info *tmp;
	struct list_head *pos;
	struct odr_pack *recv_pack;

	fd_set rset;
	int maxfd=-1;
	char *msg;
	size_t msg_len;

	msg_len = 16;
	msg = (char*)malloc(msg_len);
	buffer = malloc(ETH_PACK_SIZE);
	
	//init server socket for Unix sock. recieves request from client/server(app layer)
	un_sockfd = Socket( AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&un_server_addr, sizeof(un_server_addr));
	un_server_addr.sun_family = AF_LOCAL;
	strcpy(un_server_addr.sun_path, SRV_UN_PATH);

	unlink(un_server_addr.sun_path);
	Bind(un_sockfd, (SA*) &un_server_addr, sizeof(un_server_addr));
	maxfd = un_sockfd;
	
	//getting hw_addrs on vm into list
	INIT_LIST_HEAD(&if_info_list.list);
	ret = fill_if_info_list(&if_info_list);
	if(ret == -1){
		goto err;	
	}
	//Create socket for each hw_addr. Also getting sockfd,if_index from which to broadcast
	ret = bind_if_to_socket(&if_info_list, &maxfd);
	if(ret == -1){
		goto err;	
	}

	//Socket for ll
	ll_sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_AA19));
	//ll_sockfd = Socket( AF_LOCAL, SOCK_DGRAM, 0);
	if (setsockopt(ll_sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, 4) == -1)	{
		perror("SO_BINDTODEVICE");
		close(ll_sockfd);
		exit(EXIT_FAILURE);
	}
	
	maxfd = (ll_sockfd > un_sockfd) ? ll_sockfd: un_sockfd;	
	
	printf("max fd is %d\n", maxfd);

	FD_ZERO(&rset);
	while(1){

		FD_SET( un_sockfd, &rset);
		//list_for_each_entry_reverse(tmp, &if_info_list.list, list){
		FD_SET( ll_sockfd, &rset);
		//temp_ll_sockfd = tmp->ll_sockfd;
		//}
		printf("before select\n");
		Select(maxfd+1, &rset, NULL, NULL, NULL);
		
		printf("after select\n");
		//list_for_each_entry_reverse(tmp, &if_info_list.list, list){
		if(FD_ISSET(ll_sockfd, &rset)){
			printf("before recv\n");
			ret = recvfrom(ll_sockfd, buffer, ETH_PACK_SIZE, 0, (SA*)&recv_addr, &temp_len);
			
			recv_pack = malloc(sizeof(struct odr_pack));
			recv_pack = (struct odr_pack*) (buffer + 14);
			
			printf("payload recvd is %s\n", recv_pack->payload);
			printf("no of bytes read is %d\n", ret);
		}

		//}	

		if( FD_ISSET(un_sockfd, &rset)){
		
			printf("listening at unix sock %d\n", un_sockfd);
			ret = recvfrom(un_sockfd, msg, msg_len, 0, (SA*)&al_client_addr, &al_client_addr_size);
			if(ret == -1){
				printf("Error in recving message from applicatio layer over unix socket at client\n");
				goto err;
			}
			printf("message recvd at link layer is %s\n", msg);
			
			ret = broadcast_ll_msg(&if_info_list);
			
		}


	}



	return 0;
err:
	printf("Error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

}

