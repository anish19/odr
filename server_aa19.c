#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include "unp.h"
#include "common.h"
#include <sys/time.h>
int main(){

	int sockfd;
	int ret;
	char *odrreq;
	char time_str[32];
	int port = SERVER_PORT;
	char hostname[5];

	server2_un.sun_family = AF_LOCAL;
	strcpy(server2_un.sun_path, SRV_UN_PATH);
	
	unlink(server2_un.sun_path);
	
	sockfd = Socket( AF_LOCAL, SOCK_DGRAM, 0 );
	Bind(sockfd, (SA*) &server2_un, SUN_LEN(&server2_un));
	
	gethostname(hostname, 5);
	printf("Server Started at %s\n", hostname);
	
	while(1){
	printf("waiting for request\n");
	odrreq = (char*)malloc(2);
	ret = msg_recv_srv( sockfd, odrreq, NULL, &port);
	printf("Recieved time request.\n");
	//getting date and time
	time_t ticks = time(NULL);
	//formating data and time and writing in buf string
	snprintf(time_str, 32, "%.24s\r\n", ctime(&ticks));
	ret = msg_send_srv(sockfd, NULL, SERVER_PORT, time_str, 0 );
	printf("Sending reply to odr %s\n", time_str);
	}
	return 0;
}
