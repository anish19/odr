/*DS and Func common b/w client and odr or server and odr*/
//#include "odr.h"
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include "unp.h"
#include "hw_addrs.h"
#include "list.h"
*/
//#define CLI_UN_PATH "/tmp/kool_cli_path-XXXXXX"

#define SRV_UN_PATH "/tmp/aa19_srv_path-XXXXXX"
#define ODR_UN_PATH "/tmp/aa19_odr_path-XXXXXX"
#define SERVER_PORT 57575

char CLI_UN_PATH[100];

struct odr_reply{
	char time[32];
	char ip[15];
	int port;
};

struct odr_req{
	char msg[32];
	int forced_discovery;
};

int forced_discovery = 0;
struct sockaddr_un server_un;	//address of odr at a node
struct sockaddr_un client_un;	//address of client at node
struct sockaddr_un server2_un;	//address of server at node

int msg_send( int sockfd, char *ip, int port, char msg[], int force_discovery_flag );

int msg_send( int sockfd, char *ip, int port, char msg[], int forced_discovery_flag ){
 
     int msg_len = strlen(msg);
     int ret = 0;
     struct odr_req pack;
	 char *buffer;
	
	 strncpy(pack.msg, msg, 15);
	 pack.forced_discovery = forced_discovery_flag;
	printf("msg in pack is %s\n", pack.msg);
	 buffer = (char*)&pack;
	 forced_discovery = forced_discovery_flag;
	 //printf("msg at sento msg_send is %s\n", msg);
	 ret = sendto(sockfd, buffer, sizeof(pack), 0, (SA*)&server_un, sizeof(server_un));
     //printf("sendto msg_send ret is %d\n", ret);
	 return ret;
 
}

int msg_recv( int sockfd, char *msg, char *ip, int *port){
	
	struct odr_reply* odr_rep;
	char *buffer;
	int buf_len = sizeof(struct odr_reply);
	struct sockaddr_un odr_addr;
	int odr_addr_len;
	int ret;

	buffer = malloc(buf_len);
	ret = recvfrom(sockfd, buffer, buf_len,0 ,(SA*)&odr_addr, &odr_addr_len );
	
	odr_rep = (struct odr_reply*) buffer;
	memcpy(msg, odr_rep->time, 32);
	memcpy(ip, odr_rep->ip, 15);
	*port = odr_rep->port;
	
	return ret;
}

int msg_recv_srv(int sockfd, char *msg, char *ip, int *port){
		
		int ret;
		int odr_addr_len = sizeof(struct sockaddr_un);
		msg = (char*)malloc(2);
		ret = recvfrom(sockfd, msg, 2, 0 , (SA*)&server_un ,&odr_addr_len );
		return ret;
}

int msg_send_srv(int sockfd, char *ip, int port, char msg[], int forced_discovery_flag ){
	
	int ret;
	ret = sendto(sockfd, msg, 32, 0 , (SA*)&server_un, sizeof(struct sockaddr_un) );
	return ret;


}
