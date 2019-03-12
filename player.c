#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include "potato.h"

int main(int argc, char *argv[]){
	int port_num;
	int neigh_port;
	int playerID;
	int num_players;
	int num_hops;
	int master_fd,right_fd,left_fd;
	int status;
	int ack=1;
	char hostname[255];
	char neighname[255];
	struct addrinfo host_info;
    struct addrinfo *host_info_list;
	struct sockaddr_in playersocket;
	struct sockaddr_in neighsocket;
	socklen_t soc_len=sizeof(playersocket);
	struct hostent* player_hostentry;
	struct hostent* neigh_hostentry;
	if(argc!=3){fprintf(stderr,"wrong format,syntax:player <machine_name> <port_num>\n");exit(EXIT_FAILURE);}
	player_hostentry=gethostbyname(argv[1]);
	if(player_hostentry==NULL){fprintf(stderr,"machine name %s not found! \n",argv[1]);exit(EXIT_FAILURE);}
  port_num = atoi(argv[2]);
	if(port_num<1024||port_num>65536){fprintf(stderr,"port number should between 1024 and 65536\n");  exit(EXIT_FAILURE);}
	//connect to ringmaster
	memset(&host_info,0,sizeof(host_info));
	host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
	status = getaddrinfo(argv[1], argv[2], &host_info, &host_info_list);
	if(status!=0){fprintf(stderr,"Error: cannot get address info for host");exit(EXIT_FAILURE);}
	//create a socket to connect with ringmaster
	    master_fd=socket(host_info_list->ai_family,
                          host_info_list->ai_socktype,
                          host_info_list->ai_protocol);
	if(master_fd==-1){fprintf(stderr,"cannot create socket");exit(EXIT_FAILURE);}
	if(connect(master_fd,host_info_list->ai_addr, host_info_list->ai_addrlen)<0){perror("connect");exit(EXIT_FAILURE);}
	//create socket for player
	gethostname(hostname,sizeof(hostname));
  struct hostent* player_host=gethostbyname(hostname);
	memset(&playersocket,0,sizeof(playersocket));
	playersocket.sin_family=AF_INET;
	playersocket.sin_port=htons(port_num);
	bcopy((char*)player_hostentry->h_addr_list[0],(char*)&playersocket.sin_addr.s_addr,player_hostentry->h_length);
	int player_fd=socket(AF_INET,SOCK_STREAM,0);
	if(player_fd<0){perror("socket");exit(EXIT_FAILURE);}
	//connect to master
	//get player's portnum and send to master
	for(int i=49152;i<65535;i++){
	int	stat=bind(player_fd,(struct sockaddr*)&playersocket,sizeof(playersocket));
		playersocket.sin_port=htons(i);
       if(stat<0){continue;}
	   // memcpy(&playersocket.sin_addr, player_hostentry->h_addr_list[0], player_hostentry->h_length);
		bcopy((char*)player_hostentry->h_addr_list[0],(char*)&playersocket.sin_addr.s_addr,player_hostentry->h_length);
		if(stat==0){break;}
	}
	//send player's port number
	struct sockaddr_in buff;
	int player_port;
	memset(&buff,0,sizeof(host_info));
	if(!getsockname(player_fd,(struct sockaddr*)&buff,&soc_len)){
         player_port=ntohs(buff.sin_port);		}
		 else{perror("getsocketname");exit(EXIT_FAILURE);}
		 
	status=send(master_fd,(char*)&player_port,sizeof(player_port),0);
	if(status<0){perror("send");exit(EXIT_FAILURE);}
	//receive player's ID and player hop num
	status=recv(master_fd,&playerID,sizeof(playerID),0);
	if(status<0){perror("recv");exit(EXIT_FAILURE);}
	status=recv(master_fd,&num_hops,sizeof(num_hops),0);
	if(status<0){perror("recv");exit(EXIT_FAILURE);}
	status=recv(master_fd,&num_players,sizeof(num_players),0);
	if(status<0){perror("recv");exit(EXIT_FAILURE);}
	if(send(master_fd,(char*)&ack,sizeof(ack),0)<0){perror("send");exit(EXIT_FAILURE);}
	printf("Connected as player %d out of %d total players\n",playerID,num_players);
	//receive neigh_port num
	if(recv(master_fd,&neigh_port,sizeof(neigh_port),0)<0){perror("recv neigh_port wrong\n");exit(EXIT_FAILURE);}
	if(send(master_fd,(char*)&ack,sizeof(ack),0)<0){perror("fail to send ack");exit(EXIT_FAILURE);}
	//receive neighbor info 
	memset(neighname,0,sizeof(neighname));
	if(recv(master_fd,neighname,sizeof(neighname),0)<0){perror("recv");exit(EXIT_FAILURE);}
	//same machine localhost situation?
	neigh_hostentry=gethostbyname(hostname);
	if(neigh_hostentry==NULL){fprintf(stderr,"%s host not found\n",hostname);exit(EXIT_FAILURE);}
	//listen incomming connection
	if(listen(player_fd,2)<0){perror("listen");exit(EXIT_FAILURE);}
	//connect to the neighbor 
     left_fd=socket(AF_INET, SOCK_STREAM, 0);
	 if(left_fd<0){perror("socket");exit(EXIT_FAILURE);}
	 memset(&neighsocket,0,sizeof(neighsocket));
	 neighsocket.sin_family=AF_INET;
	 neighsocket.sin_port=htons(neigh_port);
	 bcopy((char*)neigh_hostentry->h_addr_list[0],(char*)&neighsocket.sin_addr.s_addr,neigh_hostentry->h_length);
	 if(recv(master_fd,&ack,sizeof(ack),0)<0){perror("recv");exit(EXIT_FAILURE);}
	 if(connect(left_fd,(struct sockaddr*)&neighsocket,sizeof(neighsocket))<0){perror("connect left neighbor failed");exit(EXIT_FAILURE);}
     if(send(master_fd,(char*)&ack,sizeof(ack),0)<0){perror("send");exit(EXIT_FAILURE);}
     //right accept left 
	 right_fd=accept(player_fd,(struct sockaddr*)&neighsocket,&soc_len);
	 if(right_fd<0){perror("accpet right");exit(EXIT_FAILURE);}
	 
	  srand((unsigned int) time(NULL) + playerID);
	  potato_t hotpotato;
	  fd_set readfds;
      int max_fd = 0;
	  int i=0;
	  while(1){
	//  memset(&hotpotato,0,sizeof(hotpotato));
	  FD_ZERO(&readfds);
	  FD_SET(master_fd,&readfds);
	  FD_SET(left_fd,&readfds);
	  FD_SET(right_fd,&readfds);
	  if(max_fd<master_fd){max_fd=master_fd;}
	  if(max_fd<left_fd){max_fd=left_fd;}
	  if(max_fd<right_fd){max_fd=right_fd;}
	  //check which fd change
	  if(select(max_fd+1,&readfds,NULL,NULL,NULL)<0){perror("select is wrong");exit(EXIT_FAILURE);}
	  if(FD_ISSET(master_fd,&readfds)){
		  if(recv(master_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("master_fd recv");exit(EXIT_FAILURE);}
	  }
	  if(FD_ISSET(left_fd,&readfds)){
		  if(recv(left_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("left_fd recv");exit(EXIT_FAILURE);}
	  }
	  else if(FD_ISSET(right_fd,&readfds)){
		  if(recv(right_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("right_fd recv");exit(EXIT_FAILURE);}
	  }
	  //trace
	  if(hotpotato.hopnum>0){
	  hotpotato.trace[hotpotato.curr]=playerID;
    // i++;
	  hotpotato.curr++;
	  hotpotato.hopnum--;
	  if(hotpotato.hopnum==0){
		  printf("I'm it\n");
		 if(send(master_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("fail to send back");exit(EXIT_FAILURE);}
	  }
	  else{
		  int direction=(rand())%2;
		  int next=0;
     //left
		  if(direction==0){
			  if(playerID==0){next=num_players-1;}
			  else{next=playerID-1;}
			  if(send(right_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("fail to send left");exit(EXIT_FAILURE);}
		  }
		  //right
		  else{
			  if(playerID==num_players-1){
			  next=0;}
		  else{next=playerID+1;}
		   if(send(left_fd,&hotpotato,sizeof(hotpotato),0)<0){perror("fail to send right");exit(EXIT_FAILURE);}
		  }
          printf("sending potato to %d\n",next);
	  }
	  }
	  else {close(left_fd);
	  close(right_fd);
	  close(master_fd);
    break;}
	  }
	  close(player_fd);
	  exit(EXIT_SUCCESS);
}