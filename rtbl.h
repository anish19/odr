/*routing table struct and manipulating func*/
#include <sys/time.h>
#include "odr.h"

int sp; 	//staleness parameter
struct routing_table{
	int key;
	char dest_can_ip_str[15];
	char next_hop_hwaddr[6];
	int if_no;
	int hop;
	time_t ts;
	struct list_head list;
}rtbl;

void print_routing_table(){
	
	char hostname[5];
	int self_idx;
	struct routing_table *rttmp;
	printf("\n");	
	print_self_name();
	printf(" current Routing Table is-\n\n");
	gethostname(hostname, sizeof(hostname));
	self_idx = hostname[2] - '0';
	if(hostname[2] == '1' && hostname[3] == '0')
		self_idx = 10;

	list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
		if(rttmp->key != self_idx ){
		printf("	Key: %d\n", rttmp->key);
		printf("	Destination Cannonical IP: %s\n ", rttmp->dest_can_ip_str);
		printf("	Next hop hw_addr is: ");	
			print_hwaddr(rttmp->next_hop_hwaddr);
		printf("	if_no: %d\n", rttmp->if_no);
		printf("	hops: %d\n", rttmp->hop);
		printf("	timestamp is %ld\n",rttmp->ts );
		printf("\n\n");
		}
	}
}

int entry_exists_for_src(int key){
	struct routing_table *rttmp;
	rttmp = (struct routing_table*) malloc(sizeof(struct routing_table));
	list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
		if(rttmp->key == key)
			return 1;	
	}
	return 0;
}


int update_rtbl_entry( struct odr_pack* packet, 
						char* next_hop_hwaddr,
						int sendfrom_if_idx
						){

	struct routing_table *rttmp;
	int key;

	if(packet->type == TYPE_RREQ){	
		key = get_index_from_ip(packet->src_can_ip_str);
		if(entry_exists_for_src(key) ==1 ){
			printf("Updating already existing entry in routing table\n");
			//compare and update entry
			list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
				if( rttmp->key == key){
					//check for staleness*******
					//printf("TIME - RTTMP->TS is %ld\n",time(NULL) - rttmp->ts );
					if(time(NULL) - rttmp->ts > sp){
						printf("Deleting stale entry for vm%d\n",rttmp->key);
						
						printf("Adding new entry for vm%d\n", rttmp->key);
						rttmp->key = key;
						strncpy(rttmp->dest_can_ip_str, packet->src_can_ip_str, 14);
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->ts = time(NULL);
						if(rttmp->hop > packet->rreq.hop){
							printf("New entry has updated hops\n");
							rttmp->hop = packet->rreq.hop;
						}
						return 1;
					}	
					//update if new path entry takes lesser no of hops
					if(rttmp->hop > packet->rreq.hop){
						printf("New path shows better route with lesser hop count\n");
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->hop = packet->rreq.hop;
						rttmp->ts = time(NULL);
					}
				}
			}
		}
		else{
			printf("Adding new entry to routing table\n");
			rttmp = (struct routing_table*) malloc(sizeof(struct routing_table));
			//add_new_entry
			rttmp->key = key;
			strncpy(rttmp->dest_can_ip_str, packet->src_can_ip_str, 14);
			memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
			rttmp->if_no = sendfrom_if_idx;
			rttmp->hop = packet->rreq.hop;
			rttmp->ts = time(NULL);
			list_add(&(rttmp->list), &(rtbl.list));

		}
		
		//if recvd rreq has rrep_sent bit set
		if(packet->rreq.rrep_sent == 1){
			key = get_index_from_ip(packet->src_can_ip_str);
			if(entry_exists_for_src(key) ==1 ){
				printf("Updating already existing entry to routing table when RREP_sent is set\n");
				//compare and update entry
				list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
					if( rttmp->key == key){
						//check for staleness*******
						if(time(NULL) - rttmp->ts > sp){
							printf("Deleting stale entry for vm%d\n",rttmp->key);
						
							printf("Adding new entry for vm%d\n", rttmp->key);
							rttmp->key = key;
							strncpy(rttmp->dest_can_ip_str, packet->src_can_ip_str, 14);
							memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
							rttmp->if_no = sendfrom_if_idx;
							rttmp->ts = time(NULL);
							if(rttmp->hop > packet->rreq.hop){
								printf("New entry has updated hops\n");
								rttmp->hop = packet->rreq.hop;
							}
							return 1;
						}	
						
						//update if new path entry takes lesser no of hops
						if(rttmp->hop > packet->rreq.hop){	
							printf("New path shows better route with lesser hop count\n");
							memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
							rttmp->if_no = sendfrom_if_idx;
							rttmp->hop = packet->rreq.hop;
							rttmp->ts = time(NULL);
						}
					}
				}
			}
			else{
				printf("Adding new entry to routing table\n");
				rttmp = (struct routing_table*) malloc(sizeof(struct routing_table));
				//add_new_entry
				rttmp->key = key;
				strncpy(rttmp->dest_can_ip_str, packet->dest_can_ip_str, 14);
				memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
				rttmp->if_no = sendfrom_if_idx;
				rttmp->hop = packet->rreq.hop;
				rttmp->ts = time(NULL);
				list_add(&(rttmp->list), &(rtbl.list));

			}
		}
	}
	if(packet->type == TYPE_RREP){	
		key = get_index_from_ip(packet->dest_can_ip_str);
		if(entry_exists_for_src(key) ==1 ){
			printf("Updating already existing entry to routing table\n");
			//compare and update entry
			list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
				if( rttmp->key == key){
					//check for staleness*******
					if(time(NULL) - rttmp->ts > sp){
						printf("Deleting stale entry for vm%d\n",rttmp->key);
						
						printf("Adding new entry for vm%d\n", rttmp->key);
						rttmp->key = key;
						strncpy(rttmp->dest_can_ip_str, packet->dest_can_ip_str, 14);
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->ts = time(NULL);
						if(rttmp->hop > packet->rreq.hop){
							printf("New entry has updated hops\n");
							rttmp->hop = packet->rreq.hop;
						}
						
						return 1;
					}	
					//update if new path entry takes lesser no of hops
					if(rttmp->hop > packet->rrep.hop){	
						printf("New path shows better route with lesser hop count\n");
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->hop = packet->rrep.hop;
						rttmp->ts = time(NULL);
					}
				}
			}
		}
		else{
			printf("Adding new entry to routing table\n");
			rttmp = (struct routing_table*) malloc(sizeof(struct routing_table));
			//add_new_entry
			rttmp->key = key;
			strncpy(rttmp->dest_can_ip_str, packet->dest_can_ip_str, 14);
			memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
			rttmp->if_no = sendfrom_if_idx;
			rttmp->hop = packet->rrep.hop;
			rttmp->ts = time(NULL);
			list_add(&(rttmp->list), &(rtbl.list));

		}
	}
	if(packet->type == TYPE_DATA){
	
		key = get_index_from_ip(packet->src_can_ip_str);
		if(entry_exists_for_src(key) ==1 && packet->src_dest == 1){
			printf("Updating already existing entry in routing table\n");
			//compare and update entry
			list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
				if( rttmp->key == key){
					//check for staleness*******
					//printf("TIME - RTTMP->TS is %ld\n",time(NULL) - rttmp->ts );
					if(time(NULL) - rttmp->ts > sp){
						printf("Deleting stale entry for vm%d\n",rttmp->key);
						
						printf("Adding new entry for vm%d\n", rttmp->key);
						rttmp->key = key;
						strncpy(rttmp->dest_can_ip_str, packet->src_can_ip_str, 14);
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->ts = time(NULL);
						if(rttmp->hop > packet->rreq.hop){
							printf("New entry has updated hops\n");
							rttmp->hop = packet->rreq.hop;
						}
						return 1;
					}	
					//update if new path entry takes lesser no of hops
					if(rttmp->hop > packet->rreq.hop){
						printf("New path shows better route with lesser hop count\n");
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->hop = packet->rreq.hop;
						rttmp->ts = time(NULL);
					}
				}
			}
		}
		else{
			
		}

		key = get_index_from_ip(packet->dest_can_ip_str);
		if(entry_exists_for_src(key) ==1 && packet->src_dest == -1){
			printf("Updating already existing entry in routing table\n");
			//compare and update entry
			list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
				if( rttmp->key == key){
					//check for staleness*******
					//printf("TIME - RTTMP->TS is %ld\n",time(NULL) - rttmp->ts );
					if(time(NULL) - rttmp->ts > sp){
						printf("Deleting stale entry for vm%d\n",rttmp->key);
						
						printf("Adding new entry for vm%d\n", rttmp->key);
						rttmp->key = key;
						strncpy(rttmp->dest_can_ip_str, packet->dest_can_ip_str, 14);
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->ts = time(NULL);
						if(rttmp->hop > packet->rreq.hop){
							printf("New entry has updated hops\n");
							rttmp->hop = packet->rreq.hop;
						}
						return 1;
					}	
					//update if new path entry takes lesser no of hops
					if(rttmp->hop > packet->rreq.hop){
						printf("New path shows better route with lesser hop count\n");
						memcpy(rttmp->next_hop_hwaddr, next_hop_hwaddr, 6 );
						rttmp->if_no = sendfrom_if_idx;
						rttmp->hop = packet->rreq.hop;
						rttmp->ts = time(NULL);
					}
				}
			}
		}
		else{
		}
	
	}
}

//get the routing table data for a given can_ip_addr
int get_rtbl_entry( char *entry_can_ip_addr, char *send_to_hw_addr, int *send_from_if_index, 
					int *hop){
	int key;
	struct routing_table *rttmp;
	
	key = get_index_from_ip(entry_can_ip_addr);

	if(entry_exists_for_src(key) ==1 ){
		//compare and update entry
		list_for_each_entry_reverse( rttmp, &(rtbl.list), list){
			if( rttmp->key == key){
				//check for staleness*******

				//Copy entries from routing table for user
				memcpy(send_to_hw_addr, rttmp->next_hop_hwaddr, 6 );
				*send_from_if_index = rttmp->if_no;
				*hop = rttmp->hop;
				return 1;
			}
		}
	}
	else{
		printf("Entry doesn't exist for queried address\n");
		return -1;
	}


}
