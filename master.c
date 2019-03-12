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

void errorcheck(int portnum,int playernum,int hopnum){
	if(portnum<1024||portnum>65536){fprintf(stderr,"port number should between 1024 and 65536\n");  exit(EXIT_FAILURE);}
	if(playernum<2){fprintf(stderr,"play number should larger than 2!\n");  exit(EXIT_FAILURE);}
	if(hopnum<0||hopnum>512){fprintf(stderr,"hop number should between 0 and 512 !\n");  exit(EXIT_FAILURE);}
}


int main(int argc, char *argv[]){
	int port_num;
	int num_players;
	int num_hops;
    int status;
	int stat;
    int socket_fd;
	int ack=0;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
	char hostname[255];
	if(argc!=4){fprintf(stderr,"wrong format,syntax:ringmaster <port_num> <num_players> <num_hops>\n");exit(EXIT_FAILURE);}
	port_num=atoi(argv[1]);
	num_players=atoi(argv[2]);
	num_hops=atoi(argv[3]);
	player_info playerList[num_players];
	gethostname(hostname, sizeof(hostname));
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", num_players);
  printf("Hops = %d\n", num_hops);
	//host_info=gethostbyname(hostname);
	//if(host_info==NULL){fprintf(stderr, "can't find host %s!\n", hostname);exit(EXIT_FAILURE);}
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
	//error check input format 
 errorcheck(port_num,num_players,num_hops);
  status = getaddrinfo(hostname, argv[1], &host_info, &host_info_list);
	//socket operation:create set bind listen 
	socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
	if(status==-1){perror("cannot creat socket\n");
	exit(EXIT_FAILURE);}
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if(status==-1){perror("bind"); exit(EXIT_FAILURE);}
   status = listen(socket_fd, num_players);
   if(status==-1){perror("listen");exit(EXIT_FAILURE);}
   //ringmaster connect with each player
  	for(int i=0;i<num_players;i++){
	memset(&playerList[i],0,sizeof(playerList[i]));
	struct sockaddr_in socket_player;
	//??
	int len=sizeof(socket_player);
 playerList[i].connect_fd=accept(socket_fd,(struct sockaddr *)&socket_player,&len);
	if(playerList[i].connect_fd<0){perror("accept"); exit(EXIT_FAILURE);}
    //send ID,hop number,total player num 
	int player_port;
	stat=send(playerList[i].connect_fd,&i,sizeof(i),0);//ID
	stat=send(playerList[i].connect_fd,&num_hops,sizeof(num_hops),0);//hop number
	stat=send(playerList[i].connect_fd,&num_players,sizeof(num_players),0);//player number
	if(stat==-1){perror("send");exit(EXIT_FAILURE);}
	stat=recv(playerList[i].connect_fd,&player_port,sizeof(player_port),0);
 if(stat==-1){perror("recv playerportfd fail");exit(EXIT_FAILURE);}
	stat=recv(playerList[i].connect_fd,&ack,sizeof(ack),0);
	if(stat==-1){perror("recv playerfd fail");exit(EXIT_FAILURE);}
	struct hostent* hostentry = gethostbyaddr((char *)&socket_player.sin_addr, sizeof(struct in_addr), AF_INET);
	playerList[i].listen=player_port;
	playerList[i].playerID=i;
	strcpy(playerList[i].name,hostentry->h_name);
	////ready
	printf("Player %d is ready to play\n",i);
	}
   //players connect with neighbor
   for(int i=0;i<num_players;i++){
	  // int lenn=sizeof(playerList[(i+1)%num_players].listen);
	   stat=send(playerList[i].connect_fd,(char*)&playerList[(i+1)%num_players].listen,sizeof(playerList[(i+1)%num_players].listen),0);
	   stat=recv(playerList[i].connect_fd,&ack,sizeof(ack),0);
       if(stat<0){perror("recv neigh fail");exit(EXIT_FAILURE);}
	  // int namelen=sizeof(playerList[(i+1)%num_players].name);
	   stat=send(playerList[i].connect_fd,playerList[(i+1)%num_players].name,sizeof(playerList[(i+1)%num_players].name),0);
   }
   //start neighbor connect
 int i=0;
 int tmp=1;
  while(i<num_players){
	  if(send(playerList[i].connect_fd,(char*)&tmp,sizeof(tmp),0)<0){perror("neigh connection fail\n");exit(EXIT_FAILURE);}
	  if(recv(playerList[i].connect_fd,&ack,sizeof(ack),0)<0){perror("recv back fail\n");exit(EXIT_FAILURE);}
	  i++;
  }
   //initial potato 
   potato_t hotpotato;
   memset(&hotpotato,0,sizeof(hotpotato));
   hotpotato.hopnum=num_hops;
   
   //end the game if initial hop set to 0
   if(num_hops==0){
	   for(int i=0;i<num_players;i++){
		   stat=send(playerList[i].connect_fd,(char*)&hotpotato,sizeof(hotpotato),0);
		   close(playerList[i].connect_fd);
	   }
	   close(socket_fd);
	   exit(EXIT_SUCCESS);
   };
   //start the game, send potato to random players
   srand((unsigned int)time(NULL)+num_players);
   int random=rand()%num_players;
   printf("Ready to start the game,sending potato to player %d\n",random);
   if(send(playerList[random].connect_fd, &hotpotato, sizeof(potato_t), 0) < 0);
   //launch hotpotato to random player 
//   printf("random potato hop %d\n",hotpotato.hopnum);
  // printf("player %d potato hop is %d\n",random,hotpotato.hopnum);
   //select
   int maxfd=playerList[0].connect_fd;
   fd_set readfd;
   FD_ZERO(&readfd);
   for(int i=0;i<num_players;i++){
	   FD_SET(playerList[i].connect_fd,&readfd);
	   if(maxfd<playerList[i].connect_fd){maxfd=playerList[i].connect_fd;}
   //    printf("maxfd in master %d\n",maxfd);
   }
  select(maxfd+1,&readfd,NULL,NULL,NULL);
   //catch changes
  for(int i=0;i<num_players;i++){
	  if(FD_ISSET(playerList[i].connect_fd,&readfd)){
		 int tmp=recv(playerList[i].connect_fd,&hotpotato,sizeof(hotpotato),0);
       break;

	  }
  }
   //print trace
   printf("Trace of potato:\n");
   if(num_hops==0){printf("%d\n",hotpotato.trace[0]);}
   else{
	   for(int i=0;i<num_hops;i++){
		   if(i!=num_hops-1){printf("%d,",hotpotato.trace[i]);}
		   else{printf("%d\n",hotpotato.trace[num_hops-1]);}
	   }
   }
   //end the game
   for(int i=0;i<num_players;i++){
		   stat=send(playerList[i].connect_fd,(char*)&hotpotato,sizeof(hotpotato),0);
		   close(playerList[i].connect_fd);
	   }
	   close(socket_fd);
	   exit(EXIT_SUCCESS);
}