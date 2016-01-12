#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <setjmp.h>
#include "unp.h"
//#include "unprtt.h"
#include "hw_addrs.h"
#include "list.h"
#include "common.h"

static sigjmp_buf jmpbuf;
int attempt_count = 1;

int getserveraddr(char vm_name[]){

	int ret =0;
	//char vm_name[5];
	int vm_name_size = 0;
	int i;

	printf("Enter the name of the server node(vm1-vm10).\n");
	printf("Q/q to quit client\n");
	ret = scanf("%s", vm_name);
	
	if(strncmp(vm_name,"q", 1) == 0 ||strncmp(vm_name,"Q", 1) == 0 ){
		return -2;	
	}

	i = 0;
	while( vm_name[i] != '\0' && i < 5){
//		printf("at i: %d char is %c\n", i, vm_name[i]);
		vm_name_size++;
		i++;
	}
	i = 0;

//	printf("vm_name size is %d\n", vm_name_size);
	if(ret > 0 && vm_name_size > 0){
	
		if(vm_name_size == 3){
			if(vm_name[0] == 'v' && 
				vm_name[1] == 'm' && 
				(vm_name[2] > '0' && vm_name[2] <= '9')){
			}	
			else{
				goto err;
			}
		}
		else if(vm_name_size == 4){
		
			if(vm_name[0] == 'v' && 
				vm_name[1] == 'm' && 
				vm_name[2] == '1' && 
				vm_name[3] == '0'){
			}	
			else{
				goto err;
			}
		}
		else{
			goto err;
		}
	}

//	printf("size of server name is %zu\n", strlen(vm_name));
	printf("Server name entered is : %s\n", vm_name);
	
	return 0;
	
err:
	if(ret<0)
		printf("Error in reading server name.\nPlease try again.\n");
	return -1;
	
}

static void sig_alrm(int signo)
{
	printf("signal generated\n");
//	printf("after siglngjump\n");
//	siglongjmp(jmpbuf, 1);
//	prinf("after siglongjmp\n");
}

int main(){
	//application layer communication variables
	char *client_name, *server_name;
	size_t client_name_size, server_name_size;
	struct hostent *server_addr;
	char server_addr_str[16];
	char *server_addr_ptr;
	
	//for msg_recv
	char* reply; 
	char* source_can_ip; 
	int source_port;
	//link layer communicataion variables
//	struct sockaddr_un client_un;
//	struct sockaddr_un server_un;
	fd_set fd;
	int sockfd;
	int ret = 0;
	int sel_ret = 0;
	static struct timeval select_TO;


	//getting name (application layer) of client addr
	client_name_size = 5;
	client_name = (char*)malloc(client_name_size);
	gethostname(client_name, client_name_size);
	printf("Running client on %s\n", client_name);
	
	server_name_size = 5;
	server_name = (char*)malloc(server_name_size);

	while(1){
	//get server name(applicaiton layer) from user
	ret = getserveraddr(server_name);
	if(ret == -1){
		printf("Server name entered is not correct.\nEnter b/w vm1-vm10, other than %s\n", client_name);
		errno = ENXIO;
		goto err;
	}
	if(ret == -2){
		printf("Exiting client.\n");
		exit(0);
	}

	//get cannonical server address
	server_addr = gethostbyname(server_name);
	inet_ntop(PF_INET, server_addr->h_addr_list[0], server_addr_str, sizeof(server_addr_str));
//	printf("Canonical Server addr is %s\n", server_addr_str);
	
	//init socketfd, create Unix sock and bind
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	client_un.sun_family = AF_LOCAL;
	strcpy(CLI_UN_PATH, "/tmp/kool_cli_path-XXXXXX");

	ret = mkstemp(CLI_UN_PATH);
	//ret = mkstemp(client_un.sun_family);
	if(ret == -1){
		printf("Creation of temporary sun_path failed\n");
		goto err;
	}
	strcpy(client_un.sun_path, CLI_UN_PATH);
	
	unlink(client_un.sun_path);
	Bind(sockfd, (SA*) &client_un, SUN_LEN(&client_un));
	//Bind(sockfd, (SA*) &client_un, sizeof(client_un));
	//printf("After bind client sun path is %s\n", client_un.sun_path);

	//init server addr for Unix socket.For LL communication
	bzero(&server_un, sizeof(server_un));
	server_un.sun_family = AF_LOCAL;
	strcpy(server_un.sun_path, ODR_UN_PATH);

	server_addr_ptr = (char*) malloc(16);
	strncpy(server_addr_ptr, server_addr_str, 16);

	Signal(SIGALRM, sig_alrm);
	//alarm(3);

//	ret = sendto(sockfd, server_addr_str, strlen(server_addr_str), 0, (SA*) &server_un , sizeof(server_un));
	ret = msg_send(sockfd, server_addr_ptr, SERVER_PORT, server_addr_str, 0);
	if(ret == -1){
		printf("Unable to communicate over unix socket at client\n");
		goto err;
	}
	//printf("unix socket at which msg sent is %d\n", sockfd);
	//printf("msg sent from client at al is %s\n", server_addr_str);
	printf("Client request to ODR\n");
	printf("Waiting for reply...\n");

	reply = (char*) malloc(32);
	source_can_ip = (char*) malloc(15);
	reply[0] = '\0';

	select_TO.tv_sec = 3;
	select_TO.tv_usec = 0;
	FD_ZERO(&fd);
	FD_SET(sockfd, &fd);
	sel_ret =  select(sockfd+1, &fd, NULL, NULL, &select_TO);
	if( FD_ISSET(sockfd, &fd)){
		ret = msg_recv(sockfd, reply, source_can_ip, &source_port);
		if(ret >=0)
			goto rcvd;
	}
	else{
		printf("Timed out while waiting for reply\n");	
	}
	printf("\n");
	if(sel_ret == EINTR){
		printf("select timeout\n");	
	}

/*
	if(sigsetjmp(jmpbuf, 1) != 0)
		printf("Timed out while waiting for reply\n");
		
		if(attempt_count == 2){
			alarm(0);
			printf("Server and/or ODR is possibly not running on the requested vm.\n");
			printf("Exiting...\n");
			exit(0);
			//rtt_stop(&rttinfo, rtt_ts(&rttinfo) - recvhdr.ts);
		}
*/	
	select_TO.tv_sec = 3;
	select_TO.tv_usec = 0;
	printf("Sending message from client with FORCED DISCOVERY BIT SET\n");
	ret = msg_send(sockfd, server_addr_ptr, SERVER_PORT, server_addr_str, 1);
	if(ret == -1){
		printf("Unable to communicate over unix socket at client\n");
		goto err;
	}
	printf("Client request to ODR again\n");
	printf("Waiting for reply...\n");
	attempt_count++;
	FD_ZERO(&fd);
	FD_SET(sockfd, &fd);
	sel_ret =  select(sockfd+1, &fd, NULL, NULL, &select_TO);
	if( FD_ISSET(sockfd, &fd)){
		ret = msg_recv(sockfd, reply, source_can_ip, &source_port);
		printf("Reply recieved after forced discovery\n");
	}
	else{
		printf("Timed out again\n");
		printf("Server and/or ODR is possibly not running on the requested vm.\n");
		printf("Exiting...\n");
		errno = ETIME;
		goto err;
	}
	
	
rcvd:
	printf("Reply Recieved from server\n");
	reply[30] = '\0';
	printf("\n");
	printf("Time: %s\n", reply);
	printf("Server address is: %s\n", source_can_ip);
	printf("\n-----------------------------------------------------\n\n");
	}
   return 0;
err:
	printf("Error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

}












