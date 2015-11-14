#include "common.h"

int main(){

	int ret = 0;
	int sockfd;

	struct sockaddr_un ll_server_addr;	//link layer server
	struct sockaddr_un al_client_addr;	//application layer client
	int al_client_addr_size;

	char *msg;
	size_t msg_len;

	msg_len = 16;
	msg = (char*)malloc(msg_len);

	//init server socket for Unix sock. recieves request from client/server(app layer)
	sockfd = Socket( AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&ll_server_addr, sizeof(ll_server_addr));
	ll_server_addr.sun_family = AF_LOCAL;
	strcpy(ll_server_addr.sun_path, SRV_UN_PATH);

	unlink(ll_server_addr.sun_path);
	Bind(sockfd, (SA*) &ll_server_addr, sizeof(ll_server_addr));

	ret = recvfrom(sockfd, msg, msg_len, 0, (SA*)&al_client_addr, &al_client_addr_size);
	if(ret == -1){
		printf("Error in recving message from applicatio layer over unix socket at client");
		goto err;
	}

	printf("message recvd at link layer is %s\n", msg);

	return 0;
err:
	printf("Error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

}

