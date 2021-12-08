#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


// Included to get the support library
#include <calcLib.h>
#include "protocol.h"

#define DEBUG
#define MAXDATASIZE 1401 //MAX NUM OF BYTES WE CAN GET AT ONCE


int main(int argc, char *argv[]){

  int  sockfd, numbytes;
  //char buf[MAXDATASIZE] = "Hello World\n";
  struct addrinfo hints, *servinfo, *p;
  int rv;
  
  
  struct calcMessage calcMsg;
  struct calcProtocol calcProt;
  
  //Default values as per Table 1: Client first message to server

  calcMsg.type           =  22;
  calcMsg.message        =  0;
  calcMsg.protocol       =  17;
  calcMsg.major_version  =  1;
  calcMsg.minor_version  =  0;

  

  //printf("calcProtocol = %d bytes\n",sizeof(struct calcProtocol));
  //printf("calcMessage = %d bytes\n",sizeof(struct calcMessage));
  


  calcMsg.type          = (uint16_t) ntohs(calcMsg.type);
  calcMsg.message       = (uint32_t) ntohl(calcMsg.message);
  calcMsg.protocol      = (uint16_t) ntohs(calcMsg.protocol);
  calcMsg.major_version = (uint16_t) ntohs(calcMsg.major_version);

  if (argc != 2) {
     fprintf(stderr,"usage: client hostname:port\n");
     exit(1);
    }
  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);

  #ifdef DEBUG 
  printf("Host %s, and port %s.\n",Desthost,Destport);
  #endif

  memset(&hints, 0, sizeof hints);
  
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;


  if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }    
 
  //Desthost
 
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
	  if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
	  printf("Connection successfull.\n");
        break;
    }
    /* Why are we here??? p == NULL, Looped no match (socket &| connect).. 
       Success?? */
    if (p == NULL) {
        fprintf(stderr, "client: failed to socket||connect\n");
	  /* Clear servinfo */
	  freeaddrinfo(servinfo); // all done with this structure
        return 2;
    }

    printf("Client: connected to %s:%s\n\n", Desthost,Destport);
    freeaddrinfo(servinfo); // all done with this structure

    numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, p->ai_addr, p->ai_addrlen);

    if(numbytes == -1 ){
         perror("send");
         exit(-1);
      }

    //printf("Client (%d bytes) : First message to server: calcMessage: send complete\n ",numbytes);

    numbytes = recv(sockfd, &calcProt, sizeof(calcProt), 0);
    
    if(numbytes == -1 ){
        perror("recv");
        exit(-1);
      }
    if (numbytes == 0) {
	    printf("Server closed.\n");
	    exit(-1);
      }
    //printf("Client (%d bytes) : calcProtocol message: receive complete\n",numbytes);

    if(ntohl(calcProt.arith) == 1){
      calcProt.inResult=ntohl(calcProt.inValue1)+ntohl(calcProt.inValue2);
      printf("add %u %u \n%u\n",ntohl(calcProt.inValue1),ntohl(calcProt.inValue2),calcProt.inResult);
      }else if (ntohl(calcProt.arith) == 2){
      calcProt.inResult=ntohl(calcProt.inValue1)-ntohl(calcProt.inValue2);
      printf("sub %u %u \n%u\n",ntohl(calcProt.inValue1),ntohl(calcProt.inValue2),calcProt.inResult);
      }else if (ntohl(calcProt.arith) == 3){
      calcProt.inResult=ntohl(calcProt.inValue1)*ntohl(calcProt.inValue2);
      printf("mul %u %u \n%u\n",ntohl(calcProt.inValue1),ntohl(calcProt.inValue2),calcProt.inResult);
      }else if (ntohl(calcProt.arith) == 4){
      calcProt.inResult=ntohl(calcProt.inValue1)/ntohl(calcProt.inValue2);
      printf("div %u %u \n%u\n",ntohl(calcProt.inValue1),ntohl(calcProt.inValue2),calcProt.inResult);
      }else if (ntohl(calcProt.arith) == 5){
      calcProt.flResult= calcProt.flValue1 + calcProt.flValue2;
      printf("fadd %lf %lf \n%lf\n", calcProt.flValue1, calcProt.flValue2,calcProt.flResult);
      }else if (ntohl(calcProt.arith) == 6){
      calcProt.flResult= calcProt.flValue1 - calcProt.flValue2;
      printf("fsub %lf %lf \n%lf\n", calcProt.flValue1, calcProt.flValue2,calcProt.flResult);
      }else if (ntohl(calcProt.arith) == 7){
      calcProt.flResult= calcProt.flValue1 * calcProt.flValue2;
      printf("fmul %lf %lf \n%lf\n", calcProt.flValue1, calcProt.flValue2, calcProt.flResult);
      }else if (ntohl(calcProt.arith) == 8){
      calcProt.flResult= calcProt.flValue1 / calcProt.flValue2;
      printf("fdiv %lf %lf \n%lf\n", calcProt.flValue1, calcProt.flValue2,calcProt.flResult);
    }

    calcProt.type            = 2;
    calcProt.type            = (uint16_t) ntohs(calcProt.type);
    calcProt.major_version   = 1;
    calcProt.major_version   = (uint16_t) ntohs(calcProt.major_version);
    calcProt.minor_version   = 0;
    calcProt.minor_version   = (uint16_t) ntohs(calcProt.minor_version);
    calcProt.inResult        = (int32_t) ntohl(calcProt.inResult);


    numbytes = sendto(sockfd, &calcProt, sizeof(calcProt), 0, p->ai_addr, p->ai_addrlen);
  
    if(numbytes == -1 ){
         perror("send");
         exit(-1);
      }

    //printf("Client (%d bytes) : calcProtocol message: send complete\n ",numbytes);

    numbytes = recv(sockfd, &calcMsg, sizeof(calcMsg), 0);
    
    if(numbytes == -1 ){
        perror("recv");
        exit(-1);
      }
    if (numbytes == 0) {
	    printf("Server closed.\n");
	    exit(-1);
      }
  
    //printf("Client (%d bytes) : calcMessage: receive complete\n",numbytes);
  

    if (ntohl(calcMsg.message) == 1){
    printf("%s\n","OK");
    } else if (ntohl(calcMsg.message) == 2) {
    printf("%s\n","NOT OK");
    } else if (ntohl(calcMsg.message) == 0) {
    printf("%s\n","NOT applicable/available (N/A or NA)");
    }
   
}
