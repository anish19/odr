#include "rtbl.h"
#include "common.h"
#define MAXH 15

int ll_sockfd;
int un_sockfd;
int running_broadcast_no = 1;
char server_time_reply[32];
char msg[32];
int server_port;
struct sockaddr_un al_client_addr;	//application layer client
struct sockaddr_un al_server_addr;	//application layer server
int al_client_addr_size;
int route_discovered;
//char ifname[4];

int broadcast_ll_msg( struct if_info* if_info_list, struct odr_pack* packet, int recv_if_index){
	
	int ret = 0;
	//broadcast address
	struct sockaddr_ll* bc_addr;	
	const unsigned char broadcast_addr[] = {0xff,0xff,0xff,0xff,0xff,0xff};
	struct if_info* tmp;
	char msg_str[64];
	int i;
	struct ethhdr* eh;
	void *buffer;
	char *tmch;

	buffer = malloc(ETH_PACK_SIZE);
	eh =  buffer;

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

		//filling ethernet layer packet
		memcpy(buffer, broadcast_addr, 6);
		memcpy(buffer+6, tmp->physical_addr->sll_addr, 6);
		memcpy(buffer+14, (void*)packet, sizeof(struct odr_pack));
		eh->h_proto = htons(ETH_P_AA19);
		
		printf("Broadcasting from : ");
		print_self_name();
		if(recv_if_index != tmp->physical_addr->sll_ifindex){
			ret = sendto(ll_sockfd, buffer, ETH_PACK_SIZE, 0, (SA*)bc_addr, sizeof(struct sockaddr_ll) );
			//printf("send ret is %d\n", ret);
		}
	}
	free(buffer);
	if(ret>=0)
		return 1;
	else 
		return -1;
	
}

//send the packet to sendto_hw_addr via interface idx sendfrom_if_index
int send_ll_msg(struct if_info *if_info_list, struct odr_pack* packet, char* send_to_hw_addr, int sendfrom_if_index){
	
	
	int ret = 0;
	//broadcast address
	struct sockaddr_ll* sendto_addr;	
	struct if_info* tmp;
	int i;
	struct ethhdr* eh;
	void *buffer;

	buffer = malloc(ETH_PACK_SIZE);
	eh =  buffer;

	sendto_addr = (struct sockaddr_ll*) malloc(sizeof(struct sockaddr_ll));
	list_for_each_entry_reverse( tmp, &(if_info_list->list), list){
		
		//setting send address
		sendto_addr->sll_family = AF_PACKET;
		sendto_addr->sll_ifindex = sendfrom_if_index;
		sendto_addr->sll_halen = IF_HADDR;
		sendto_addr->sll_protocol = htons(ETH_P_AA19);
		memcpy(sendto_addr->sll_addr, send_to_hw_addr, IF_HADDR);
		sendto_addr->sll_addr[6] = 0x00;	
		sendto_addr->sll_addr[7] = 0x00;	

		//filling ethernet layer packet
		memcpy(buffer, send_to_hw_addr, 6);
		memcpy(buffer+6, tmp->physical_addr->sll_addr, 6);
		memcpy(buffer+14, (void*)packet, sizeof(struct odr_pack));
		eh->h_proto = htons(ETH_P_AA19);
		
		if(sendfrom_if_index == tmp->physical_addr->sll_ifindex){
			printf("Sending from : ");
			print_self_name();
			printf("Sending to HW addr : ");
			print_hwaddr(send_to_hw_addr);
			ret = sendto(ll_sockfd, buffer, ETH_PACK_SIZE, 0, (SA*)sendto_addr, sizeof(struct sockaddr_ll) );
			//printf("send ret is %d\n", ret);
		}
	}
	if(ret>=0)
		return 1;
	else 
		return -1;
}

int send_payload(struct if_info *if_info_list, struct odr_pack *packet, int key){
	struct routing_table *rttmp;
	int ret=0;
	list_for_each_entry_reverse(rttmp, &(rtbl.list), list){
		if(rttmp->key == key){
			ret = send_ll_msg(if_info_list, packet, rttmp->next_hop_hwaddr, rttmp->if_no);
		}
	}
	//printf("send payload ret is %d\n", ret);
	if(ret >=0)
		return 1;
	else
		return -1;
}

int fill_if_info_list(struct if_info* if_info_list ){
	
	struct if_info *tmp;
	struct hwa_info *hwahead, *hwa;
	char hostname[5];
	
	gethostname(hostname, sizeof(hostname));

	hwahead = (struct hwa_info*) malloc(sizeof(struct hwa_info));
	hwa = (struct hwa_info*) malloc(sizeof(struct hwa_info));
	printf("-------------------------\n");
	printf("\t  %s\n", hostname);
	printf("-------------------------\n");
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next ){
	
		printf("	Interface name is: %s\n", hwa->if_name);
		printf("	Hardware address is: " ) ;
		print_hwaddr (hwa->if_haddr);
		printf("	Interface index is: %d\n", hwa->if_index);
		printf("	Interface ip_alias is: %d\n", hwa->ip_alias);
		printf("	Interface IP addr is: %s\n", Sock_ntop(hwa->ip_addr, sizeof(SA*)) );
		printf("\n");
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
		//	strncpy(ifname, hwa->if_name, 4);
		}
	}
	printf("-------------------------\n");

	return 1;
}

int is_self_dest(char dest_ip_str[]){

	char self_ip_str[14];
	char self_name[5];
	struct hostent *self_addr;
	int ret = 0;
	int i;
	int match = 1;

	ret = gethostname(self_name, 5);
	if(ret == -1)
		return -1;
	//printf("ret from gethost name is %d\n", ret);
	//printf("self name is %s\n", self_name);	
	
	self_addr = gethostbyname(self_name);
	inet_ntop(PF_INET, self_addr->h_addr_list[0], self_ip_str, sizeof(self_ip_str));
	//printf("Canonical Server addr is %s\n", self_ip_str);

	match = strncmp(self_ip_str, dest_ip_str, 14 );
	if(match == 0){
		//printf("string matched\n");
		return 1;
	}
	else 
		return 0;
}

int is_self_src(char src_ip_str[]){

	char self_ip_str[14];
	char self_name[5];
	struct hostent *self_addr;
	int ret = 0;
	int i;
	int match = 1;

	ret = gethostname(self_name, 5);
	if(ret == -1)
		return -1;
	//printf("ret from gethost name is %d\n", ret);
	//printf("self name is %s\n", self_name);	
	
	self_addr = gethostbyname(self_name);
	inet_ntop(PF_INET, self_addr->h_addr_list[0], self_ip_str, sizeof(self_ip_str));
	//printf("Canonical Server addr is %s\n", self_ip_str);

	match = strncmp(self_ip_str, src_ip_str, 14 );
	if(match == 0){
		//printf("string matched\n");
		return 1;
	}
	else 
		return 0;
}


//src_hw_addr is the hardware address from which we recvd the packet. 
//In case of dest node, we recv RREQ from src_hw_addr and sent RREP to this
int process_pack( struct if_info *if_info_list, struct odr_pack* packet, char* src_hw_addr, int recv_if_index){
	int ret = 0;
	int src_idx;
	int key;
	char *send_to_hw_addr;	//send out RREQ/RREP/DATA to this hw_addr
	int send_from_if_index;		//send out through this interface
	int ulv = 0;
	int dest_key;
	send_to_hw_addr = (char*) malloc(6);

	if(packet->rreq.hop > MAXH || packet->rrep.hop > MAXH){
		return 1;
	}
	switch(packet->type){
		case TYPE_RREQ:
			printf("Recieved a RREQ with Broadcast id : %d\n", packet->rreq.broadcast_id);
			printf("Source for recieved RREQ is : vm%d\n", get_index_from_ip(packet->src_can_ip_str));
			
			update_rtbl_entry( packet, src_hw_addr, recv_if_index);
			if( packet->rreq.rrep_sent == 1){
				printf("RREP_ALREADY_SENT Flag is set on recieved RREQ. Not forwarding this RREQ\n");
				break;
			}
			ret = is_self_dest(packet->dest_can_ip_str);
			printf("Cannonical IP addr for server is :%s\n", packet->dest_can_ip_str);
			printf("MAC address from where packet was recieved is :");
			print_hwaddr(src_hw_addr);
			
			printf("Hops till this point: %d\n", packet->rreq.hop);
			if(ret == 1){
				printf("RREQ has reached odr at server\n");
				printf("Creating RREP\n");
				packet->type = TYPE_RREP;
				packet->rrep.hop = 1;
				packet->rrep.broadcast_id = 1;
				packet->rrep.force_discovery = 0;
				
				//query routing table and send ***
				printf("Determining best path to send RREP to client from the routing table.\n");
				ret = get_rtbl_entry(packet->src_can_ip_str, send_to_hw_addr, &send_from_if_index, &ulv );
				//send via path provided by rtbl if rtbl entry exists
				if(ret == -1){
					printf("Route table entry for source not found in server\n");
					//send back through the path from which it came
					//ret = send_ll_msg(if_info_list, packet, src_hw_addr, recv_if_index);
				}
				else{
					printf("Route table entry for source found.\n");
					printf("Sending RREP from server\n");
					ret = send_ll_msg(if_info_list, packet, send_to_hw_addr, send_from_if_index);
				}
			}
			else{
				//printf("not reached destination yet\n");
				printf("Recieved RREQ is at an intermidiate node\n");
				//recording that a specific RREQ has been recieved
				//we find the index of the source node by get_index_from_ip
				src_idx = get_index_from_ip( packet->src_can_ip_str);
				
				//consider fwding only if the RREQ recvd is unique(different (source, broadcast_id))
				if(latest_rreq[src_idx] < packet->rreq.broadcast_id){
					//printf("inside with new broadcast id\n");
					printf("Recieved RREQ is unique\n");
					latest_rreq[src_idx] = packet->rreq.broadcast_id;
					packet->rreq.hop++;
					//if routing table entry for dest doesnt exist,then broadcast
					if( (entry_exists_for_src( get_index_from_ip( packet->dest_can_ip_str)) == 0) || 
						forced_discovery == 1){
						
						printf("BROADCASTING RREQ from Intermediate node, because no route table entry or forced dircovery\n");
						ret = broadcast_ll_msg(if_info_list, packet, recv_if_index);
					}
					else{
					//else send rrep with destination
						printf("Route table entry for server FOUND at this Intermediate node\n");
						//query routing table and send ***
						ret = get_rtbl_entry(packet->dest_can_ip_str, send_to_hw_addr, &send_from_if_index, &packet->rrep.hop );
						//MAKE rrep packet
						packet->type = TYPE_RREP;
						//packet->rreq.rrep_sent = 1;
						printf("Hops for RREQ till this node are : %d\n", packet->rreq.hop);
						packet->rrep.hop++;
						packet->rrep.broadcast_id = 1;
						packet->rrep.force_discovery = 0;
												
						//send via path provided by rtbl if rtbl entry exists
						if(ret == -1){
							printf("Routing table entry for server doesnot exist.\n");
						//	ret = send_ll_msg(if_info_list, packet, src_hw_addr, recv_if_index);
						}
						else{
						//else send back through the path from which it came
							//ret = broadcast_ll_msg(if_info_list, packet, -1);
							printf("Sending RREP from Intermediate node, because route table entry for server exists\n");
							ret = send_ll_msg(if_info_list, packet, send_to_hw_addr, send_from_if_index);
							
							printf("Broadcasting RREQ from this node with RREP_ALREADY_SENT set to 1\n");
							//broadcast rreq with rrep_flag set
							packet->type = TYPE_RREQ;
							packet->rreq.hop++;
							packet->rreq.rrep_sent = 1; 
							ret = broadcast_ll_msg(if_info_list, packet, -1);
						}
					}
				}
				else{
					printf("Recieved RREQ is duplicate. This will be ignored after updating routing table.\n");
				}
			}
			break;
		case TYPE_RREP:
			//find route from table and fwd
			printf("RREP recieved\n");

			ret = is_self_src(packet->src_can_ip_str);
			//update table
			//printf("HW ADDR from where pack was recv is :");
			//print_hwaddr(src_hw_addr);
			update_rtbl_entry( packet, src_hw_addr, recv_if_index);
			
			if(ret == 1){
				if(route_discovered == 0){
				printf("RREP recieved at client\n");
				printf("Hops required for RREP is %d\n", packet->rrep.hop);
				dest_key = get_index_from_ip(packet->dest_can_ip_str);
				printf("Routing table entry for server %s found.\n", packet->dest_can_ip_str);
				printf("SENDING PAYLOAD from client\n");
				packet->type = TYPE_DATA;
				strncpy(packet->payload, msg, 32);
				packet->src_dest = 1;
				ret = send_payload(if_info_list, packet, dest_key);
				//printf("ret after send_payload in TYPE RREP process payload is %d\n", ret);
				route_discovered = 1;
				}
			}
			else{
				printf("RREP recieved at intermediate node\n");
				src_idx = get_index_from_ip( packet->src_can_ip_str);
				
				//fwd RREP
				//if routing table entry for src doesnt exist
				printf("Checking route table for entry for source\n");
				if( entry_exists_for_src( get_index_from_ip( packet->src_can_ip_str)) == 0){
					printf("\n\n****ALERT 1 while fwding RREP*****\n\n");
					printf("No route table entry found.\n");
					//printf("BROADCASTING from Intermediate node, because no route table entry\n");
					printf("This has probably occured because a vm was restarted.\n");
					printf("Fix: Restart all vms or increase staleness parameter\n");
					//ret = broadcast_ll_msg(if_info_list, packet, recv_if_index);
				}
				else{
					//else fwd rrep to next node
					printf("Route table entry for client(source) exists\n");
					printf("Forwarding RREP from Intermediate node.\n");
					//MAKE rrep packet
					packet->rrep.hop++;
					packet->rrep.force_discovery = 0;
					
					//query routing table and send ***
					ret = get_rtbl_entry(packet->src_can_ip_str, send_to_hw_addr, &send_from_if_index, &ulv );
					//send via path provided by rtbl if rtbl entry exists
					if(ret == -1){
						//ret = send_ll_msg(if_info_list, packet, src_hw_addr, recv_if_index);
						printf("Routing table entry not found for forwarding RREP.\n");
					}
					else{
						//ret = broadcast_ll_msg(if_info_list, packet, -1);
						ret = send_ll_msg(if_info_list, packet, send_to_hw_addr, send_from_if_index);
					}
				}
				
			}
			break;
		case TYPE_DATA:
			printf("Packet recieved type is PAYLOAD\n");
			//update table
			update_rtbl_entry( packet, src_hw_addr, recv_if_index);
			
			if(is_self_src(packet->src_can_ip_str)){
				printf("*********************************************\n");
				printf("Reply recieved from server\n");
				//printf("Hops required : %d\n", packet->rrep.hop);
				//printf("Reply recieved at client\n");	
				printf("Reply from server is : %s\n", packet->payload);
				printf("*********************************************\n");
				
				//send message to client****
				struct odr_reply odr_rep;
				char* buffer;
				buffer = (char*)malloc(sizeof(struct odr_reply));
				memcpy(odr_rep.time, packet->payload, 32);
				memcpy(odr_rep.ip, packet->dest_can_ip_str, 15);
				odr_rep.port = server_port;
				memcpy(buffer, &odr_rep, sizeof(struct odr_reply));
				//printf("buffer = %s\n", buffer);
				//printf("un_sockfd = %d\n", un_sockfd);
				
				//printf("client_un sunpath is %s\n", client_un.sun_path);
				//printf("client_un sunpath is %s\n", al_client_addr.sun_path);
				ret = sendto(un_sockfd, buffer, sizeof(struct odr_reply), 0, (SA*)&al_client_addr, sizeof(al_client_addr));
				//ret = sendto(un_sockfd, buffer, sizeof(struct odr_reply), 0, (SA*)&client_un, sizeof(client_un));
				//printf("ret at sendto is %d\n", ret);
				if(ret>0){
					printf("Processed packet has been sent to client\n");
				}
				else
					printf("Unable to send the processed packet to client\n");
			}
			else if(is_self_dest(packet->dest_can_ip_str)){
				printf("*********************************************\n");
				printf("Time request recieved at server\n");
				printf("Processing client's request\n");
				
				char* timereq;
				int addr_size = sizeof(struct sockaddr_un);
				timereq = (char*)malloc(2);
				strncpy(timereq, "tt", 2);
				
				al_server_addr.sun_family = AF_LOCAL;
				strcpy( al_server_addr.sun_path, SRV_UN_PATH);
						
				printf("Sending time request to server\n");
				ret = sendto(un_sockfd, timereq, 2, 0, (SA*) &al_server_addr, sizeof(al_server_addr));
				
				fd_set fd;
				static struct timeval odr_to;
				odr_to.tv_sec = 3;
				odr_to.tv_usec = 0;
				FD_ZERO(&fd);
				FD_SET(un_sockfd, &fd);
				Select(un_sockfd+1, &fd, NULL, NULL, &odr_to);
				printf("Waiting for reply from server\n..");
				if(FD_ISSET(un_sockfd, &fd)){
					ret = recvfrom(un_sockfd, server_time_reply, 32, 0, (SA*)&al_server_addr, &addr_size);				
				}
				else{
					printf("Timeout while waiting for reply from server.\nMake sure server process is running on this node\n");
					return 1;
				}
				printf("Recieved reply from server\n");
/*			
				//getting date and time
				time_t ticks = time(NULL);
				//formating data and time and writing in buf string
				snprintf(server_time_reply, sizeof(server_time_reply), "%.24s\r\n", ctime(&ticks));
*/				
				packet->src_dest = -1;				
				strncpy(packet->payload, server_time_reply, 32);
				printf("Sending reply\n");
				src_idx = get_index_from_ip(packet->src_can_ip_str);
				ret = send_payload(if_info_list, packet, src_idx);
				//printf("ret from send payload towards client at server is %d\n", ret);
				printf("Reply sent is: %s\n", server_time_reply);
				printf("*********************************************\n");
			}
			else{
				//Forwarding PAYLOAD in appropriate direction at intermidiate node
				printf("PAYLOAD recieved at intermediate node\n");
				if(packet->src_dest == 1){
					dest_key = get_index_from_ip(packet->dest_can_ip_str);
					if( entry_exists_for_src( dest_key) == 0){
						printf("\n\n****ALERT 2 while fwding RREP*********\n\n");
						//printf("PAYLOAD1 from Intermediate node, because no route table entry\n");
						printf("Routing table entry for source not found at vm.\n");
						printf("This has probably occured because a vm was restarted.\n");
						printf("Fix: Restart all vms\n");
					//	packet->type = TYPE_RREQ;
					//	ret = broadcast_ll_msg(if_info_list, packet, recv_if_index);
					}
					else{
						printf("Forwarding Payload towards server\n");
						ret = send_payload(if_info_list, packet, dest_key);
					}
				}
				
				if(packet->src_dest == -1){
					src_idx = get_index_from_ip(packet->src_can_ip_str);
					if( entry_exists_for_src( src_idx) == 0){
						printf("\n\n****ALERT 3 while fwding RREP*********\n\n");
						printf("Routing table entry for source not found at vm.\n");
						//printf("PAYLOAD-1 from Intermediate node, because no route table entry\n");
						printf("This has probably occured because a vm was restarted.\n");
						printf("Fix: Restart all vms\n");
						//packet->type = TYPE_RREQ;
						//ret = broadcast_ll_msg(if_info_list, packet, recv_if_index);
					}
					else{
						printf("Forwarding Payload towards client\n");
						ret = send_payload(if_info_list, packet, src_idx);
					}
				}
			}
			break;
		default:
			printf("Packet type not identified.\n");
			printf("Exiting.\n");
			ret = -1;
			break;
	}
	return ret;

}

int main(int argc, char *argv[]){

	int ret = 0;
	int i = 0;	

	//cannonical destination address
	struct sockaddr_in *dest_can_ip;
	char* dest_can_ip_str;
	char* src_can_ip_str;
	int dest_key;
	char* src_hw_addr;

	//odr packet to use;
	struct odr_pack source_packet;

	//for temp recv
	struct sockaddr_ll *recv_addr;
	void *buffer;
	void* r_buf;
	struct odr_req* odr_req_pack;
	int temp_len;

	//unix socket
	struct sockaddr_un un_server_addr;	//link layer server

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

	al_client_addr_size = sizeof(struct sockaddr_un);
	fd_set rset;
	int maxfd=-1;
	size_t msg_len;

	if(argc>2){
		printf("Too many arguments\n");
		errno = EINVAL;
		goto err;
	}
	if(argc == 1){
		printf("Staleness parameter not entered.\nTry again.\n");
		errno = EINVAL;
		goto err;
	}
	sp = atoi(argv[1]);
	printf("Staleness parameter is : %d\n", sp);

	src_can_ip_str = (char*) malloc(15);
	msg_len = 32;
	//msg = (char*)malloc(msg_len);
	buffer = malloc(ETH_PACK_SIZE);
	recv_addr = (struct sockaddr_ll*) malloc(sizeof(struct sockaddr_ll));
	//init server socket for Unix sock. recieves request from client/server(app layer)
	un_sockfd = Socket( AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&un_server_addr, sizeof(un_server_addr));
	un_server_addr.sun_family = AF_LOCAL;
	strcpy(un_server_addr.sun_path, ODR_UN_PATH);

	//printf("Before bind sun_path %s\n", un_server_addr.sun_path);
	unlink(un_server_addr.sun_path);
	Bind(un_sockfd, (SA*) &un_server_addr, sizeof(un_server_addr));
	//printf("After bind sun_path %s\n", un_server_addr.sun_path);
	//Connecting
	//al_client_addr.sun_family = AF_LOCAL;
	//strcpy(al_client_addr.sun_path, CLI_UN_PATH);
	//Connect(un_sockfd, (SA*) &al_client_addr, sizeof(struct sockaddr_un));

	maxfd = un_sockfd;
	server_port = 57575;
	//init latest_rreq 
	for(i=0;i<11;i++)
		latest_rreq[i] = 0;
	i = 0;

	//init routing table
	INIT_LIST_HEAD(&rtbl.list);

	//getting hw_addrs on vm into list
	INIT_LIST_HEAD(&if_info_list.list);
	ret = fill_if_info_list(&if_info_list);
	if(ret == -1){
		goto err;	
	}
	
	//Socket for ll
	ll_sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_AA19));
	
	maxfd = (ll_sockfd > un_sockfd) ? ll_sockfd: un_sockfd;	
	
	FD_ZERO(&rset);
	while(1){

		FD_SET( un_sockfd, &rset);
		FD_SET( ll_sockfd, &rset);
		
		printf("----------------------------------------\n");
		print_routing_table();
		printf("----------------------------------------\n");
		//printf("before select\n");
		printf("Waiting...\n");
		Select(maxfd+1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(ll_sockfd, &rset)){
			//printf("before recv\n");
			temp_len = sizeof(struct sockaddr_ll);
			ret = recvfrom(ll_sockfd, buffer, ETH_PACK_SIZE, 0, (SA*)recv_addr, &temp_len);
			//printf("no of bytes read is %d\n", ret);

			//hw_addr of vm from where the packet came
			src_hw_addr = malloc(6);
			memcpy(src_hw_addr, buffer+6, 6);
			
			recv_pack = malloc(sizeof(struct odr_pack));
			recv_pack = (struct odr_pack*) (buffer + 14);
			
			ret = process_pack(&if_info_list ,recv_pack, src_hw_addr, recv_addr->sll_ifindex);
			if(ret == -1){
				printf("Error in processing packet recieved by odr\n");
				goto err;
			}
			
			//printf("payload recvd is %s\n", recv_pack->payload);
			//printf("dest addr recvd is %s\n", recv_pack->dest_can_ip_str);
		}


		if( FD_ISSET(un_sockfd, &rset)){
			
			int r_buf_len;
			route_discovered = 0;
			odr_req_pack = (struct odr_req*) malloc(sizeof(struct odr_req));
			r_buf = malloc(sizeof(struct odr_req));
			r_buf_len = sizeof(struct odr_req);
			//printf("listening at unix sock %d\n", un_sockfd);
			ret = recvfrom(un_sockfd, r_buf, r_buf_len, 0, (SA*)&al_client_addr, &al_client_addr_size);
			if(ret == -1){
				printf("Error in recving message from applicatio layer over unix socket at client\n");
				goto err;
			}
			odr_req_pack = (struct odr_req*) r_buf;
			strncpy(msg, odr_req_pack->msg, 32);
			forced_discovery = odr_req_pack->forced_discovery;
			//printf("size of sun_path is %d\n", sizeof(al_client_addr.sun_path));
			//printf("AL client addr is : %s\n", al_client_addr.sun_path );
			//printf("message recvd at link layer is %s\n", msg);
			printf("Message recieved from client at odr.\n");
			printf("Msg rcvd is %s\n", msg);

			dest_can_ip = (struct sockaddr_in*) malloc(sizeof(SA));
			inet_aton(msg, &dest_can_ip->sin_addr );
			dest_can_ip->sin_family = AF_INET;
			dest_can_ip->sin_port = SERVER_PORT;
			
			dest_can_ip_str = (char*) malloc(15);
			dest_can_ip_str = Sock_ntop((SA*)dest_can_ip, sizeof(SA));
			//printf("cannonical IP rcved at odr is %s\n", Sock_ntop((:SA*)dest_can_ip, sizeof(SA)) );
			
			i=0;
			while(dest_can_ip_str[i] != ':'){
				i++;
			}
			dest_can_ip_str[i] = '\0';
			
			get_can_ip(src_can_ip_str);
			for(i = 0; i< 14; i++)
				source_packet.dest_can_ip_str[i] = dest_can_ip_str[i];
			source_packet.dest_can_ip_str[14] = '\0';
			i = 0;

			for(i = 0; i< 14; i++){
				source_packet.src_can_ip_str[i] = src_can_ip_str[i];
			}
			source_packet.src_can_ip_str[14] = '\0';
			source_packet.type = TYPE_RREQ;
			
			int skip_discovery = 0;
			printf("Forced Discovery flag is %d\n", forced_discovery);
			//check if routing table entry already exists, send payload if it does
			//ret = update_rtbl_entry(&source_packet, , );
			dest_key = get_index_from_ip(dest_can_ip_str);
			if(entry_exists_for_src(dest_key) && forced_discovery == 0){
				printf("Routing table entry for %s ALREADY EXISTS\n", dest_can_ip_str);
				printf("Sending PAYLOAD ...\n");
				source_packet.type = TYPE_DATA;
				source_packet.src_dest = 1;
	//			strncpy(source_packet.payload, msg, msg_len);
				ret = send_payload(&if_info_list, &source_packet, dest_key);
				printf("PAYLOAD SENT from client.\n");
				skip_discovery = 1;			
			}

			//get_can_ip(dest_can_ip_str);

			//printf("dest_ip_can_str is %s\n", dest_can_ip_str);
			source_packet.type = TYPE_RREQ;
			source_packet.rreq.hop = 1;
			source_packet.rreq.broadcast_id = running_broadcast_no++;
			source_packet.rreq.force_discovery = 0;
			source_packet.rreq.rrep_sent = 0;
			source_packet.rrep.hop = -1;
			source_packet.rrep.broadcast_id = -1;
			source_packet.rrep.force_discovery = -1;
			latest_rreq[get_index_from_ip(src_can_ip_str)] = running_broadcast_no;
			//printf("src_can_ip_str in packet is %s\n", source_packet.src_can_ip_str);
			i=0;
			//broadcast from all interface
			if(skip_discovery == 0){
				printf("Routing table entry for server does not exist at client node\n");
				printf("Broadcasting RREQs\n");
				ret = broadcast_ll_msg(&if_info_list, &source_packet, -1 );
			}

		}


	}


	printf("Exiting successfully\n");
	return 0;
err:
	printf("Error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

}
