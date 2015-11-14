#include "common.h"

int getserveraddr(char vm_name[]){

	int ret =0;
	//char vm_name[5];
	int vm_name_size = 0;
	int i;

	printf("Enter the name of the server node(vm1-vm10).\n");
	ret = scanf("%s", vm_name);

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
	printf("Server name entered is %s\n", vm_name);
	
	return 0;
	
err:
	if(ret<0)
		printf("Error in reading server name.\nPlease try again.\n");
	return -1;
	
}


int main(){
	//application layer communication variables
	char *client_name, *server_name;
	size_t client_name_size, server_name_size;
	struct hostent *server_addr;
	char server_addr_str[16];
	
	//link layer communicataion variables
	struct sockaddr_un client_un;
	struct sockaddr_un server_un;
	
	int sockfd;
	int ret = 0;

	//getting name (application layer) of client addr
	client_name_size = 5;
	client_name = (char*)malloc(client_name_size);
	gethostname(client_name, client_name_size);
	printf("Running client on %s\n", client_name);
	
	server_name_size = 5;
	server_name = (char*)malloc(server_name_size);
	
	//get server name(applicaiton layer) from user
	ret = getserveraddr(server_name);
	if(ret == -1){
		printf("Server name entered is not correct.\nEnter b/w vm1-vm10, other than %s\n", client_name);
		errno = ENXIO;
		goto err;
	}

	//get cannonical server address
	server_addr = gethostbyname(server_name);
	inet_ntop(PF_INET, server_addr->h_addr_list[0], server_addr_str, sizeof(server_addr_str));
	printf("Canonical Server addr is %s\n", server_addr_str);
	
	//init socketfd, create Unix sock and bind
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	client_un.sun_family = AF_LOCAL;
	strcpy(client_un.sun_path, CLI_UN_PATH);

	ret = mkstemp(client_un.sun_path);
	if(ret == -1){
		printf("Creation of temporary sun_path failed\n");
		goto err;
	}
	
	unlink(client_un.sun_path);
	Bind(sockfd, (SA*) &client_un, sizeof(client_un));

	//init server addr for Unix socket.For LL communication
	bzero(&server_un, sizeof(server_un));
	server_un.sun_family = AF_LOCAL;
	strcpy(server_un.sun_path, SRV_UN_PATH);

	ret = sendto(sockfd, server_addr_str, strlen(server_addr_str), 0, (SA*) &server_un , sizeof(server_un));
	if(ret == -1){
		printf("Unable to communicate over unix socket at client\n");
		goto err;
	}
	printf("msg sent from client at al is %s\n", server_addr_str);


   return 0;
err:
	printf("Error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);

}












