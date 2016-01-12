CSE 533: Network Programming
On Demand Routing
--------------------------------------
Anish Ahmed 
SBU ID: 110560809
Netid: aniahmed
Pratik Shrivastav
SBU ID: 110385785
Netid: pshrivastav cse533-6
--------------------------------------

ABOUT
-----
The on demand routing system at link layer runs on all vms to discover the best path from client to server and back. It uses RAW SOCKETS to communicate b/w machines connected on the network. With this communication route table entrites are obtained a nodes. Using these entries the client request is forwarded to the server via the best (minimum hop distance) path. The server replies with the current time. This time is relyed back to the client odr process via link layer communicaiton and finally displayed.


FILES INCLUDED
--------------
1. Makefile

2. common.h
3. odr.h
4. rtbl.h
5. hw_addr.h
6. list.h

7. client_aa19.c
8. server_aa19.c
9. odr_aa19.c
10. get_hw_addrs_aa19.c
11. prhwaddrs_aa19.c


COMPILATION and RUNNING
-----------------------
Run - 
	make
To compile the required files.


Run-
	./deploy_app odr_aa19 client_aa19 server_aa19
To deploy the apps on all vms.


1. Start odr_aa19 on all vms by ./odr_aa_19
2. Start client_aa19 on the client vm by ./client_aa19
3. Start server_aa19 on the server vm by ./server_aa19


Run- 
	make clean to remove all the .o and excecutables.

BEHAVIOUR
--------------
- At the client side the user starts the client process and enters the server from which he/she wants to get time.
- The client process sends the request to odr process running on that vm.
- odr process checks if there is a routing table entry for the required server node.
- If a routing table entry exixts, payload containing time request is sent to server.
- If routing table entry doesn't exist for the required server, route discovery is performed.
- In route discovery, RREQs are flooded out of the client and every intermidiate vm.
- If an intemediate vm knows the route to the server vm then it replies with an RREP to the client.
- The client then adds that entry to its routing table and sends out Payload with time request to server.
- If no intermidiate vm has an entry for the server then all intermediate vms flood out RREQ. 
- When an RREQ is recieved at the destination, it replies with an RREP.
- When the RREP reached the client by following the reverse path, the client finishes route discovery.
- After this the client sends out the payload with time request to the server.
- When server odr recieves the time request, it forwards the request to the server process.
- The server process calculates the current time and sends back the time on the best known path to the client.
- The client odr process recieves the packet from server and forwards it to the client process.
- The client process then prints out the time.


SYSTEM DOCUMENTATION
--------------------

Source node refers to the vm on which client is running.
Destination node refers to the vm on whcih the server is running.


DATA STRUCTURE:

The hardware interface information for each vm is stored in a list whose elements are of the following structure - 
	
	struct if_info{
		struct sockaddr_ll* physical_addr;
		struct sockaddr* ip_addr;
		char if_name[IF_NAME];
		int ll_sockfd;
		struct list_head list;
	};


The routing table entries are stored in a list, whose entries are of the following structure - 

	struct routing_table{
		int key;
		char dest_can_ip_str[15];
		char next_hop_hwaddr[6];
		int if_no;
		int hop;
		time_t ts;
		struct list_head list;
	}rtbl;

- Route table entries are updates when the latest packet recieved corresponding to a specific vm has lower hop count then the already recorded one.
- Route table entires are deleted whenever a stale entry is found.
- New route table entries are added if there is not entry for the correspondgin node mentioned in the packet.

list.h is used to create list based on the list provided by Linux Kernel.
(list.h has been picked up from http://www.mcs.anl.gov/~kazutomo/list/list.h)


ODR:

When a node recieves a packet 3 cases are possible-
	1. The node is an intermidiate node.
		SUBCASES-
		Packet recieved is a-
			
			RREQ: 
				- Route table entry is updated.
				- If route table entry exists for destination then RREP is sent back to the client
			
			RREP:
				- Route table is updated.
				- Route table is looked up to find the next hop hardware address and the interface number.
				- THe packet is forwarded to the hardware address obtained from route table lookup. 
			
			PAYLOAD:
				- Route table is updated.
				- Route table is looked up to find the next hop hardware address and the interface number.
				- THe packet is forwarded to the hardware address obtained from route table lookup. 

	2. The node is the source node.(client)
		SUBCASES-
		Packet recieved is a-
			RREQ:
				- Route table is updated.
				- The RREQ recieved is always a duplicate RREQ being broadcasted back. 
				- This will be ignored.

			RREP:
				- Route table is updates.
				- Source now knows the path to the server.
				- Source sends out the Payload with time request.

			PAYLOAD:
				- This will occur when the server reply has reached the source.
				- The source extracts the reply message from the packet and forwards it to the client process
				- The client prints out the time. 

	3. The node is the destinaiton node.(server)
		SUBCASES-
		Packet recieved is a-
			RREQ:
				- Route table is updated.
				- RREP is sent back to the client.

			RREP:
				- Never recieves a RREP

			PAYLOAD:
				- This payload will contain the client request.
				- The server will forward the request to the server program
				- The sever waits for reply from the client process.
				- The server times out if no reply is obtained within 3 secs.
				- If reply is obtained then the sever puts the reply in a packet and forwards it to the next hop node towards the client.


The client-odr and server-odr communication is through unix domiain sockets


CLIENT:
	- The client waits for input from the user
	- The user specifies the vm from which it wants to request time.
	- The request is sent over unix domian socket to the odr running at client.
	- odr forwards the request if it has a routing table entry for the source 
	- If routing table entry for destination doesnot exist it performs route discovery.
	- After the odr at client recieves reply from the sever it forwards the reply message to the client.
	- The client prints out the time
	- The client again waits for a user input.
	- The client can be exited by 'q' or 'Q'


SERVER
	- The server is bound to a weel known unix domain path and port.
	- The server is waiting for a request from client. 
	- This request comes via odr running at server node.
	- The request is obtained via a unix domain socket.
	- The server calculates the time and replies to the unix address of odr at server.
	- The odr at server recieves the reply from server process and forwards it to the client.


























