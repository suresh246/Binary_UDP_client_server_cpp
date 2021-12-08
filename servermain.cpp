#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

/* You will to add includes here */

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"



// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG
#define BACKLOG 1	 // how many pending connections queue will hold
#define SECRETSTRING "gimboid"

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate=0;


/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum){
  // As anybody can call the handler, its good coding to check the signal number that called it.

  printf("Let me be, I want to sleep, loopCount = %d.\n", loopCount);

  if(loopCount>20){
    printf("I had enough.\n");
    terminate=1;
  }
  
  return;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}







int main(int argc, char *argv[]){
  
   /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
	
  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 

  /* Do magic */
  int port=atoi(Destport);
  #ifdef DEBUG  
  printf("Host %s, and port %d.\n",Desthost,port);
  #endif

  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  #define PORT "5000"  // the port users will be connecting to

  struct calcMessage calcMsg,rcMsg;
  struct calcProtocol calcProt,rcProt;
  
  //Default values as per Table 1: Client first message to server

  calcMsg.type           =  22;
  calcMsg.message        =  0;
  calcMsg.protocol       =  17;
  calcMsg.major_version  =  1;
  calcMsg.minor_version  =  0;

  

  printf("calcProtocol = %d bytes\n",sizeof(struct calcProtocol));
  printf("calcMessage = %d bytes\n",sizeof(struct calcMessage));
  


  calcMsg.type          = (uint16_t) ntohs(calcMsg.type);
  calcMsg.message       = (uint32_t) ntohl(calcMsg.message);
  calcMsg.protocol      = (uint16_t) ntohs(calcMsg.protocol);
  calcMsg.major_version = (uint16_t) ntohs(calcMsg.major_version);


    /* Initialize the library, this is needed for this library. */
  initCalcLib();
  char *ptr;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
  
  	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}



	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections BINJER...\n");
	char msg[1500];
	char msg_result[1500];
	int j, MAXSZ=sizeof(msg);

	int  childCnt=0;
	int  numbytes;
	char command[10];
	char temp1[20], temp2[20];
	char optionstring[128];
	int optionint1;
	int optionint2;

  numbytes = recv(sockfd, &rcMsg, sizeof(rcMsg), 0);
    
  if(numbytes == -1 ){
        perror("recv");
        exit(-1);
      }
  if (numbytes == 0) {
	    printf("Server closed.\n");
	    exit(-1);
      }
  
  printf("Server (%d bytes) : calcMessage: receive complete\n",numbytes);


  if ( rcMsg.major_version == calcMsg.major_version 
        & rcMsg.message == calcMsg.message 
        & rcMsg.minor_version == calcMsg.minor_version 
        & rcMsg.protocol == calcMsg.protocol 
        & rcMsg.type == calcMsg.type){
    
    printf("major_version: %u\n",htons(rcMsg.major_version));
    printf("message: %u\n",htonl(rcMsg.message));
    printf("minor_version: %u\n",htons(rcMsg.minor_version));
    printf("protocol: %u\n",htons(rcMsg.protocol));
    printf("type: %u\n",htons(rcMsg.type));

/*
  uint16_t type;  // What message is this, 1 = server to client, 2 client to server, 3... reserved , conversion needed 
  uint16_t major_version; // 1, conversion needed 
  uint16_t minor_version; // 0, conversion needed 
  uint32_t id; // Server side identification with operation. Client must return the same ID as it got from Server., conversion needed 
  uint32_t arith; // What operation to perform, see mapping below. 
  int32_t inValue1; // integer value 1, conversion needed 
  int32_t inValue2; // integer value 2, conversion needed 
  int32_t inResult; // integer result, conversion needed 
  double flValue1;  // float value 1,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms        
  double flValue2;  // float value 2,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms
  double flResult;  // float result,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms
*/


	  ptr=randomType(); // Get a random arithemtic operator. 

    double f1,f2,fresult;
    int i1,i2,iresult,iid,arth;

    i1=randomInt();
    i2=randomInt();
    iid=randomInt();

    f1=randomFloat();
    f2=randomFloat();

    printf("Int Values: %d %d \n", i1,i2);
    printf("Float Values: %8.8g %8.8g \n",f1,f2);
    
    calcProt.type            =   2;
    calcProt.type            =   (uint16_t) htons(calcProt.type);
    calcProt.major_version   =   1;
    calcProt.major_version   =   (uint16_t) htons(calcProt.major_version);
    calcProt.minor_version   =   0;
    calcProt.minor_version   =   (uint16_t) htons(calcProt.minor_version);
    calcProt.id              =   iid;                                              
    calcProt.id              =   (uint32_t) htonl(calcProt.id);

		
		/* Act differently depending on what operator you got, judge type by first char in string. If 'f' then a float */
  
    if(ptr[0]=='f'){
      /* At this point, ptr holds operator, f1 and f2 the operands. Now we work to determine the reference result. */
    if(strcmp(ptr,"fadd")==0){
        arth = 5;
        fresult=f1+f2;
        } else if (strcmp(ptr, "fsub")==0){
            arth = 6;
            fresult=f1-f2;
        } else if (strcmp(ptr, "fmul")==0){
            arth = 7;
            fresult=f1*f2;
        } else if (strcmp(ptr, "fdiv")==0){
            arth = 8;
            fresult=f1/f2;
        }

        printf("%s %d %8.8g %8.8g = %8.8g\n",ptr,arth, f1,f2,fresult);
		    sprintf(msg, "%s %8.8g %8.8g",ptr, f1, f2);
		    sprintf(msg_result,"%8.8g\n",fresult);

        calcProt.arith           =   arth;      
        calcProt.arith           =   (uint32_t) htonl(calcProt.arith);      
        calcProt.inValue1        =   0;
        calcProt.inValue1        =   (int32_t)  htonl(calcProt.inValue1);
        calcProt.inValue2        =   0;
        calcProt.inValue2        =   (int32_t)  htonl(calcProt.inValue2);
        calcProt.inResult        =   0;
        calcProt.inResult        =   (int32_t)  htonl(calcProt.inResult);
        calcProt.flValue1        =   f1;
        calcProt.flValue1        =   (double)   htonl(calcProt.flValue1);
        calcProt.flValue2        =   f2;
        calcProt.flValue2        =   (double)   htonl(calcProt.flValue2);
        calcProt.flResult        =   0;
        calcProt.flResult        =   (double)   htonl(calcProt.flResult);
  



    numbytes = sendto(sockfd, &calcProt, sizeof(calcProt), 0, p->ai_addr, p->ai_addrlen);
  
    if(numbytes == -1 ){
      perror("send");
      exit(-1);
    }

    printf("Server (%d bytes) : calcProtocol message: send complete\n ",numbytes);

    /* arith mapping in calcProtocol
     1 - add
     2 - sub
     3 - mul
     4 - div
     5 - fadd
     6 - fsub
     7 - fmul
     8 - fdiv

     other numbers are reserved

   */   

    }
    else {
          if(strcmp(ptr,"add")==0){
             arth = 1;
             iresult=i1+i2;
        } else if (strcmp(ptr, "sub")==0){
             arth = 2;
             iresult=i1-i2;
        } else if (strcmp(ptr, "mul")==0){
             arth = 3;
             iresult=i1*i2;
        } else if (strcmp(ptr, "div")==0){
             arth  = 4;
             iresult=i1/i2;
        }
		  
        printf("%s %d %d %d = %d \n",ptr,arth,i1,i2,iresult);
		    sprintf(msg,"%s %d %d",ptr,i1,i2);
		    sprintf(msg_result,"%d\n",iresult);


        calcProt.arith           =   arth;      
        calcProt.arith           =   (uint32_t) htonl(calcProt.arith);      
        calcProt.inValue1        =   i1;
        calcProt.inValue1        =   (int32_t)  htonl(calcProt.inValue1);
        calcProt.inValue2        =   i2;
        calcProt.inValue2        =   (int32_t)  htonl(calcProt.inValue2);
        calcProt.inResult        =   0;
        calcProt.inResult        =   (int32_t)  htonl(calcProt.inResult);
        calcProt.flValue1        =   0;
        calcProt.flValue1        =   (double)   htonl(calcProt.flValue1);
        calcProt.flValue2        =   0;
        calcProt.flValue2        =   (double)   htonl(calcProt.flValue2);
        calcProt.flResult        =   0;
        calcProt.flResult        =   (double)   htonl(calcProt.flResult);

        numbytes = sendto(sockfd, &calcProt, sizeof(calcProt), 0, p->ai_addr, p->ai_addrlen);
  
        if(numbytes == -1 ){
          perror("send");
          exit(-1);
        }

        printf("Server (%d bytes) : calcProtocol message: send complete\n ",numbytes);

    
  }

        }

  else {

  /* 
   calcMessage.type
   1 - server-to-client, text protocol
   2 - server-to-client, binary protocol
   3 - server-to-client, N/A
   21 - client-to-server, text protocol
   22 - client-to-server, binary protocol
   23 - client-to-serve, N/A
   
   calcMessage.message 

   0 = Not applicable/availible (N/A or NA)
   1 = OK   // Accept 
   2 = NOT OK  // Reject 

  */

  //Default values as per Table 1: Client first message to server

  calcMsg.type           =  2;
  calcMsg.message        =  2;
  calcMsg.protocol       =  17;
  calcMsg.major_version  =  1;
  calcMsg.minor_version  =  0;

  

  //printf("calcProtocol = %d bytes\n",sizeof(struct calcProtocol));
  //printf("calcMessage = %d bytes\n",sizeof(struct calcMessage));
  


  calcMsg.type          = (uint16_t) ntohs(calcMsg.type);
  calcMsg.message       = (uint32_t) ntohl(calcMsg.message);
  calcMsg.protocol      = (uint16_t) ntohs(calcMsg.protocol);
  calcMsg.major_version = (uint16_t) ntohs(calcMsg.major_version);


  numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, p->ai_addr, p->ai_addrlen);

  if(numbytes == -1 ){
    perror("send");
    exit(-1);
  }

  }


  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 

  #ifdef DEBUG 
  printf("Host %s, and port %s.\n",Desthost,Destport);
  #endif


  
  while(terminate==0){
    printf("This is the main loop, %d time.\n",loopCount);
    sleep(1);
    loopCount++;
  }

  printf("done.\n");
  return(0);


  
}
