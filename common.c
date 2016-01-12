#include "common.h"

int msg_send( int sockfd, char *ip, int port, char msg[], int force_discovery_flag ){

	int msg_len = strlen(msg);
	int ret = 0;
	ret = sendto(sockfd, msg, msg_len, 0, (SA*)server_un, sizeof(server_un));
	return ret;

}
