/*
 ** selectserver.c -- a cheezy multiperson chat server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#define PORT "5001"   // port we're listening on
#define STDIN 0





void getIP(const struct addrinfo *p ,char ip[INET6_ADDRSTRLEN]){
    void *addr;
    // get the pointer to the address itself,
    // different fields in IPv4 and IPv6:
    if (p->ai_family == AF_INET) { // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
        
    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);			
    }    
    char ip1[INET6_ADDRSTRLEN];
    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, addr, ip1, sizeof ip1);
    strcpy(ip,ip1);	
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



// Send a File to a sockNumber
void sendFile(int sockNum,FILE* pFileSend){
    struct timeval t1,t2;
    double speed;
    double elapsedTime;
    gettimeofday(&t1,NULL);
	int i;
	char sendBuffer[256];
	fseek(pFileSend,0,SEEK_END);
	long int fileSize = ftell(pFileSend);  //Get file Size
    fseek(pFileSend,0,SEEK_SET);
	bzero(sendBuffer,sizeof(sendBuffer));
    //Send the files using loop
	for(i=0;i<fileSize/256;i++){
		memset(sendBuffer,0,sizeof(sendBuffer));
		fread(sendBuffer,1,256,pFileSend);
		if ((send(sockNum, sendBuffer, sizeof(sendBuffer), 0)) == -1) 
		{
   			perror("1");
			exit(1);
        }	
	}
    //Send remaining bytes
	memset(sendBuffer,0,sizeof(sendBuffer));
	fread(sendBuffer,1,fileSize%256,pFileSend);
	if ((send(sockNum, sendBuffer, fileSize%256, 0)) == -1)
    {
        perror("1");
        exit(1);
    }
    //Send the end msg
	memset(sendBuffer,0,sizeof(sendBuffer));
	strcpy(sendBuffer,"end");
	if ((send(sockNum, sendBuffer, sizeof(sendBuffer), 0)) == -1) 
    {
        perror("1");
        exit(1);
    }	
    gettimeofday(&t2,NULL);  
    elapsedTime = (t2.tv_sec - t1.tv_sec ) * 1000.0;    //Measure Time
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    speed = fileSize/1000000.0/elapsedTime * 1000.0;     // Measure Speed
    printf("Send Time : %f msecs , FileSize %f MB, Speed %f MB per Sec \n", elapsedTime , fileSize/1000000.0, speed);
    
    
    printf("File Send Successful!\n");
	fclose(pFileSend);	
	
}



//Recv a File from a socket number
void recvFile(int sockNum,FILE* pFileRecv,long int fileSize){
    struct timeval t1,t2;
    double elapsedTime;
    gettimeofday(&t1,NULL);
    double speed;
	int i;
	char recvBuffer[257];
   // printf("Before For Loop\n");
    //Recv the file in this loop
	for(i=0;i<fileSize/256;i++){
		bzero(recvBuffer,sizeof(recvBuffer));
		if(recv(sockNum, recvBuffer, 256, 0)==-1){
			perror("recv");
			exit(1);
		}
        
		if(!strcmp(recvBuffer,"end")){
			break;
		} else{
			recvBuffer[256] = '\0';
			fwrite(recvBuffer,sizeof(recvBuffer[0]),256,pFileRecv);
            //fputs(recvBuffer,pFileRecv);
		}
	}
    
	bzero(recvBuffer,sizeof(recvBuffer));
	if(recv(sockNum, recvBuffer, fileSize%256, 0)==-1){
        perror("recv");
        exit(1);
    }
	if(fileSize%256 != 0){
        fwrite(recvBuffer,sizeof(recvBuffer[0]),fileSize%256,pFileRecv);
        //recvBuffer[(fileSize%256) -1] = '\0'; 
    }
	gettimeofday(&t2,NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec ) * 1000.0;  //Measure Time
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;  
    
	
    
    
	
	fclose(pFileRecv);	
    
    
    
    speed = fileSize/1000000.0 / elapsedTime * 1000.0;   //Measure Speed;
    printf("Recv Time : %f msecs , FileSize %f MB, Speed %f MB per Sec \n", elapsedTime , fileSize/1000000.0, speed);
    
    
    printf("File Received\n");
    fflush(stdout);
	
}


//Get the count of white space of a string , Example   "CMD 1" would return 1 and "CMD" would return 0
int strprt(const char* str){
	int i=0;
	int count = 0;
	for(i=0;i<strlen(str)-1;i++){
		if(*(str+i) == ' ') count++;
		//printf("1\n");
    }
	return count;
}


//The information of the peers
struct peer
{ 
    int used;
    int sockNum;
  //  char peerName[128];
    char ip[INET6_ADDRSTRLEN];
    
};











int main(void)
{
    //At most 5 peers supported
    struct peer peers[5];    
    
    //If the program is transfering data, 0=NO 1=YES
    int transferstate = 0;
   // fflush(stdout);
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    char command[100]; // command line
    int nbytes;  
    
    //Start of listener's block
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    
    char bufRecv[256];    // for file receiving     
    char remoteIP[INET6_ADDRSTRLEN];
    
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    char peerName[128];
    
	struct addrinfo hints, *ai, *p;
    
    //End of listener's side
    
    //Start of talker's side
    struct addrinfo hintsT, *servinfo, *pT;
	int rvT;
	int sockfd;
	char localName[128];
    
    
	
    
    //printf("Size %d",sizeof(peers)/sizeof);
    for(i=0;i<5;i++){
        peers[i].used = 0;
        peers[i].sockNum = 0;
        //bzero(peers[i].sockNum,sizeof(peers[i].sockNum));
        //bzero(peers[i].peerName,sizeof(peers[i].peerName));
        bzero(peers[i].ip,sizeof(peers[i].ip));
        
    }
    
    //Get local ip, save to ip4 variable
    char ip4[INET_ADDRSTRLEN];
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    
    getifaddrs(&ifAddrStruct);
    
    
    
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            if(strcmp(addressBuffer,"127.0.0.1")){
                strcpy(ip4,addressBuffer);
            }
        } 
    }
   // printf("MYIP %s\n",ip4);
    //Get IP

    
    //End of talker's side
    gethostname(localName,128);
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    
	// get us a socket and bind it
    //listener
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
    //talker
	memset(&hintsT, 0, sizeof hintsT);
	hintsT.ai_family = AF_UNSPEC;
	hintsT.ai_socktype = SOCK_STREAM;
    
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        
		if (listener < 0) { 
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
        
		break;
	}
    
	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
    
	freeaddrinfo(ai); // all done with this
    
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    
    // add the listener to the master set
    FD_SET(listener, &master);
    FD_SET(STDIN, &master);
    
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    
    // main loop
    
    for(;;) {
        
        //printf("Main Loop \n");
        if(transferstate == 0)
            //  printf("Enter Msg:");
            
            fflush(stdout);
        FD_SET(STDIN,&master);
        read_fds = master; // copy it
        
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax + 1; i++) {
            //printf("In fdmax %d\n",i);
            
            if(FD_ISSET(STDIN,&read_fds)){
                //The user typed something !
                fgets(command,100,stdin);
                command[strlen(command)-1]='\0';
                //Command is download
                if(0!=strstr(command,"DOWNLOAD")){
                    printf("ZhuoChen\n");
                    transferstate = 1;
                    int parts = strprt(command);
                    //Format is DOWNLOAD 0 1.txt
                    if(2==parts){
                        //Make a folder
                        mkdir("downLoad",S_IREAD | S_IWRITE | S_IEXEC);
                        int cntNumber;
                        char tempStr1[64];
                        char fileName1[128];
                        char msg1[128];
                        char msgBuffer1[128];
                        long int fileSize1;
                        strcpy(fileName1,"downLoad/");
                        printf("FileName %s\n",fileName1);
                        FILE* pFileWrite1;
                        sscanf(command,"%*s %d %s",&cntNumber,tempStr1);
                        strcat(fileName1,tempStr1);
                        //   sprintf(msg1,"DOWNLOAD %s",fileName1);
                        /*   if(send(peers[cntNumber].sockNum,msg1,sizeof(msg1),0)==-1){
                         perror("send");
                         } */
                        
                        if(!strcmp(strstr(tempStr1,"."),".txt")) pFileWrite1 = fopen(fileName1,"w");
                        else{pFileWrite1 = fopen(tempStr1,"wb"); }
                        //Send the message format DOWNLOAD 1.txt
                        sprintf(msg1,"DOWNLOAD %s",tempStr1);
                        
                        
                        if(send(peers[cntNumber].sockNum,msg1,sizeof(msg1),0)==-1){
                            perror("send");
                            exit(4);
                        }
                        else{
                            if(recv(peers[cntNumber].sockNum,msgBuffer1,sizeof(msgBuffer1),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer1,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    //File Size
                                    sscanf(msgBuffer1,"%*s %ld",&fileSize1);   
                                   // printf("File Size %ld\n",fileSize1);
                                   // printf("The Msg is %s\n",msgBuffer1);
                                    
                                    //Begin to recv file
                                    recvFile(peers[cntNumber].sockNum,pFileWrite1,fileSize1);
                                }
                            }
                        }
                        transferstate = 0;                        
                    }	else if(4==parts){
                        //DOWNLOAD format is DOWNLOAD 0 1.txt 1 2.txt
                        mkdir("downLoad",S_IREAD | S_IWRITE | S_IEXEC);
                        int cntNumber1;
                        int cntNumber2;
                        char tempStr1[64];
                        char tempStr2[64];
                        char fileName1[128];
                        char fileName2[128];
                        char msg1[128];
                        char msg2[128];
                        char msgBuffer1[128];
                        char msgBuffer2[128];
                        long int fileSize1;
                        long int fileSize2;
                        FILE* pFileWrite1;
                        FILE* pFileWrite2;
                        
                        strcpy(fileName1,"downLoad/");
                        strcpy(fileName2,"downLoad/");
                        sscanf(command,"%*s %d %s %d %s",&cntNumber1,tempStr1,&cntNumber2,tempStr2);
                        strcat(fileName1,tempStr1);
                        strcat(fileName2,tempStr2);
                        
                        
                        
                        //   sprintf(msg1,"DOWNLOAD %s",fileName1);
                        /*  if(send(peers[cntNumber1].sockNum,msg1,sizeof(msg1),0)==-1){
                         perror("send");
                         } */
                        
                        if(!strcmp(strstr(fileName1,"."),".txt")) pFileWrite1 = fopen(fileName1,"w");
                        else{pFileWrite1 = fopen(fileName1,"wb"); }
                        
                        if(!strcmp(strstr(fileName2,"."),".txt")) pFileWrite2 = fopen(fileName2,"w");
                        else{pFileWrite2 = fopen(fileName2,"wb"); }
                        
                        
                        sprintf(msg1,"DOWNLOAD %s",tempStr1);
                        if(send(peers[cntNumber1].sockNum,msg1,sizeof(msg1),0)==-1){
                            perror("send");
                            exit(4);
                        }
                        
                        else{
                            if(recv(peers[cntNumber1].sockNum,msgBuffer1,sizeof(msgBuffer1),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer1,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    sscanf(msgBuffer1,"%*s %ld",&fileSize1);
                                    recvFile(peers[cntNumber1].sockNum,pFileWrite1,fileSize1);
                                }
                            }
                        }
                        
                        sprintf(msg2,"DOWNLOAD %s",tempStr2);
                        if(send(peers[cntNumber2].sockNum,msg2,sizeof(msg2),0)==-1){
                            perror("send");
                            exit(4);
                        } else{
                            if(recv(peers[cntNumber2].sockNum,msgBuffer2,sizeof(msgBuffer2),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer2,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    sscanf(msgBuffer2,"%*s %ld",&fileSize2);
                                    recvFile(peers[cntNumber2].sockNum,pFileWrite2,fileSize2);
                                }
                            }
                        }
                        
                        
                        transferstate = 0;       
                    } else if(6==parts){
                        //DOWNLOAD format is DOWNLOAD 0 1.txt 1 2.txt 2 3.txt
                        mkdir("downLoad",S_IREAD | S_IWRITE | S_IEXEC);
                        int cntNumber1;
                        int cntNumber2;
                        int cntNumber3;
                        char tempStr1[64];
                        char tempStr2[64];	
                        char tempStr3[64];
                        char fileName1[128];
                        char fileName2[128];
                        char fileName3[128];	
                        char msg1[128];
                        char msg2[128];
                        char msg3[128];
                        char msgBuffer1[128];
                        char msgBuffer2[128];
                        char msgBuffer3[128];
                        long int fileSize1;
                        long int fileSize2;
                        long int fileSize3;
                        FILE* pFileWrite1;
                        FILE* pFileWrite2;
                        FILE* pFileWrite3;
                        
                        
                        strcpy(fileName1,"downLoad/");
                        strcpy(fileName2,"downLoad/");
                        strcpy(fileName3,"downLoad/");
                        sscanf(command,"%*s %d %s %d %s %d %s",&cntNumber1,tempStr1,&cntNumber2,tempStr2,
                               &cntNumber3,tempStr3);
                        strcat(fileName1,tempStr1);
                        strcat(fileName2,tempStr2);
                        strcat(fileName3,tempStr3);
                        
                        //   sprintf(msg1,"DOWNLOAD %s",fileName1);
                        /*  if(send(peers[cntNumber1].sockNum,msg1,sizeof(msg1),0)==-1){
                         perror("send");
                         } */
                        
                        if(!strcmp(strstr(fileName1,"."),".txt")) pFileWrite1 = fopen(fileName1,"w");
                        else{pFileWrite1 = fopen(fileName1,"wb"); }
                        
                        if(!strcmp(strstr(fileName2,"."),".txt")) pFileWrite2 = fopen(fileName2,"w");
                        else{pFileWrite2 = fopen(fileName2,"wb"); }
                        
                        if(!strcmp(strstr(fileName3,"."),".txt")) pFileWrite3 = fopen(fileName3,"w");
                        else{pFileWrite3 = fopen(fileName3,"wb"); }
                        
                        
                        sprintf(msg1,"DOWNLOAD %s",tempStr1);
                        if(send(peers[cntNumber1].sockNum,msg1,sizeof(msg1),0)==-1){
                            perror("send");
                            exit(4);
                        }
                        
                        else{
                            if(recv(peers[cntNumber1].sockNum,msgBuffer1,sizeof(msgBuffer1),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer1,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    sscanf(msgBuffer1,"%*s %ld",&fileSize1);
                                    recvFile(peers[cntNumber1].sockNum,pFileWrite1,fileSize1);
                                }
                            }
                        }
                        
                        sprintf(msg2,"DOWNLOAD %s",tempStr2);
                        if(send(peers[cntNumber2].sockNum,msg2,sizeof(msg2),0)==-1){
                            perror("send");
                            exit(4);
                        } 
                        else{
                            if(recv(peers[cntNumber2].sockNum,msgBuffer2,sizeof(msgBuffer2),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer2,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    sscanf(msgBuffer2,"%*s %ld",&fileSize2);
                                    recvFile(peers[cntNumber2].sockNum,pFileWrite2,fileSize2);
                                }
                            }
                        }
                        
                        sprintf(msg3,"DOWNLOAD %s",tempStr3);
                        if(send(peers[cntNumber3].sockNum,msg3,sizeof(msg3),0)==-1){
                            perror("send");
                            exit(4);
                        } 
                        else{
                            if(recv(peers[cntNumber3].sockNum,msgBuffer3,sizeof(msgBuffer3),0)==-1){
                                perror("recv");
                                exit(4);
                            }
                            else{
                                if(!strcmp(msgBuffer3,"ERROR")){perror("Error Recvd");exit(4); }
                                else{
                                    sscanf(msgBuffer3,"%*s %ld",&fileSize3);
                                    recvFile(peers[cntNumber3].sockNum,pFileWrite3,fileSize3);
                                }
                            }
                        }
                        
                        
                        transferstate = 0;       
                    }
                    
                    else{
                        perror("Bad Command");
                        exit(4);
                    }
                }
                
                
                else
                { 
                    if(0 == command[0]) break;
                    switch(strprt(command)){
                        case 0:
                            
                            //Help Information
                            if(!strcmp("HELP",command)){
                                printf("Help typed\n\n");
                                printf("MYIP to get local IP\n\n");
                                printf("CONNECT name port to connect to remote peer\n");
                                printf("Example : CONNECT 128.205.36.8 5001\n\n");
                                
                                printf("MYPORT to get port number\n\n");
                                                                
                                printf("LIST to list all peers\n\n");
                                
                                printf("CREATOR to display developer Information\n\n");
                                
                                printf("TERMINATE cntNumber to terminate a connection\n");
                                printf("Example : TERNIMATE 0      note: the 0 is the connection number\n");
                                printf("You can use LIST to find connection number\n\n");
                                
                                printf("UPLOAD cntNumber FileName to upload a file\n");
                                printf("Example UPLOAD 0 1.txt\n");
                                printf("You can only upload a file to a connected peer\n\n");
                                
                                printf("DOWNLOAD cntNumber FileName to download a file\n");
                                printf("Example DOWNLOAD 0 1.txt\n");
                                printf("You can only download from a connected peer\n");
                                printf("At most three file can be downloaded simutanously\n");
                                printf("Example DOWNLOAD 0 1.txt 1 2.txt 2 3.txt\n\n");
                                
                                
                                memset(command,0,sizeof(command));
                                break;
                                
                            }   
                            //MY IP
                            else if(!strcmp("MYIP",command)){
                                printf("MYIP\n");
                                printf("MY IP is %s\n",ip4);
                              
                            }   else if(!strcmp("MYPORT",command)){
                                printf("MY PORT %s", PORT);
                                bzero(command,sizeof(command));
                            }
                            //LIST
                            else if(!strcmp("LIST",command)){
                                printf("LIST\n");
                                memset(command,0,sizeof(command));
                                for(i=0;i<5;i++){
                                    if(1==peers[i].used){
                                        printf("Connection Number %d\n",i);
                                        printf("Socket Number %d\n",peers[i].sockNum);
                                      //  printf("Peer Name %s\n",peers[i].peerName);
                                        printf("Peer IP %s\n",peers[i].ip);
                                    }
                                }			    
                                break;
                            }   
                            else if(!strcmp("EXIT",command)){
                                //QUIT
                                exit(4);
                                printf("Exit!");
                                
                            } else if(!strcmp("CREATOR",command)){
                                //Who is the author?
                                printf("Creator : Zhuo Chen \n UBMail:zhouchen@buffalo.edu\n");
                                fflush(stdout);
                                bzero(command,sizeof(command));
                                printf("Exit!");
                                
                            }
                            else{printf("Bad Command\n"); break;memset(command,0,sizeof(command));}
                            
                            
                            
                        case 1:
                            
                            if(0!=strstr(command,"TERMINATE")){
                                //Terminate a connection
                                int tmtNum;
                                sscanf(command,"%*s %d",&tmtNum);
                                close(tmtNum);
                                peers[tmtNum].used = 0;
                              //  bzero(peers[tmtNum].peerName,sizeof(peers[tmtNum].peerName));
                                bzero(peers[tmtNum].ip,sizeof(peers[tmtNum].ip));
                                
                                // printf("cntCmd: %s\n",cntCmd);
                                //   printf("cntAddr: %s\n",cntAddr);
                                //  printf("portNmb: %d\n",portNumber);
                                break;
                            }
                        case 2:
                            //Connect
                            if(0!=strstr(command,"CONNECT")){
                                //connect to a peer
                                //  printf("Connecting\n");
                                char cntCmd[10];
                                char cntAddr[128];
                                char portNumber[10];
                                sscanf(command,"%*s %s %s",cntAddr,portNumber);
                                if ((rv = getaddrinfo(cntAddr, portNumber, &hintsT, &servinfo)) != 0) {
                                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                                    return 1;
                                }
                                
                                
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
                                    char ip[INET6_ADDRSTRLEN];
                                    getIP(p,ip);
                                    //Add a connectable peer
                                    for(i=0;i<5;i++){
                                        if(peers[i].used==0){
                                            peers[i].used = 1;
                                            peers[i].sockNum = sockfd;
                                            //strcpy(peers[i].peerName,
                                            strcpy(peers[i].ip,ip);
                                            break;
                                        }
                                        
                                    }
                                    
                                    break;
                                }
                                
                                
                                if ((sockfd, localName, strlen(localName), 0) == -1) {
                                    perror("send");
                                }
                                if(sockfd>fdmax){
                                    fdmax = sockfd;
                                }
                                //if(send(sockfd,"SendYouAName",20,0)==-1){perror("send"); }
                                
                                
                                
                                // printf("cntCmd: %s\n",cntCmd);
                                //  printf("cntAddr: %s\n",cntAddr);
                                //  printf("portNmb: %d\n",portNumber);
                                break;
                            } else if(0!=strstr(command,"UPLOAD")){ //Upload
                                
                                transferstate = 1;
                                int cntNum;
                                char fileName[128];
                                char msgSend[128];
                                //Upload to cntNumber, upload the file with fileName
                                sscanf(command,"%*s %d %s",&cntNum,fileName);
                                if(cntNum>9 || 1!=peers[cntNum].used){
                                    perror("Bad Cnt Number\n");
                                    exit(4);
                                }
                                
                                FILE* pFileRead;
                                if(!strcmp(strstr(fileName,"."),".txt")) pFileRead = fopen(fileName,"r");
                                else pFileRead = fopen(fileName,"rb");
                                if(NULL == pFileRead){perror("File Dont Exist!\n");exit(4);}
                                fseek(pFileRead,0,SEEK_END);
                                long int fileSize = ftell(pFileRead);  //File Size
                                fseek(pFileRead,0,SEEK_SET);
                                sprintf(msgSend,"UPLOAD NAME %s SIZE %ld",fileName,fileSize);
                                if(send(peers[cntNum].sockNum,msgSend,sizeof(msgSend),0) == -1){
                                    perror("send");
                                }  //Send the msg with the format UPLOAD NAME 1.txt SIZE 512
                                
                                sendFile(peers[cntNum].sockNum,pFileRead);  //Send the File
                            } //Uploadß
                            transferstate = 0;
                            
                            
                    }
                    FD_CLR(STDIN,&read_fds);
                }
            }
            
            
            
            /*		if(!strcmp("HELP",command)){
             printf("Help typed\n");
             memset(command,0,sizeof(command));
             }    
             else(!strcmp("CONNECT",command)){
             printf("CONNECT
             }            
             FD_CLR(STDIN, &read_fds);
             } */
            else if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;    //The accept will change the remoteAddr
					newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);
                    //printf("after accept\n");
                    
					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        char currentIP[128];
                        strcpy(currentIP,inet_ntop(remoteaddr.ss_family,
                                                   get_in_addr((struct sockaddr*)&remoteaddr),
                                                   remoteIP, INET6_ADDRSTRLEN));
                        if(0!=strstr(currentIP,ip4) || 0!=strstr(currentIP,"127.0.0.1")) 
                        {
                            printf("Self Connection!\n");
                            return 0;
                        }
                        for(j=0;j<5;j++){
                            if(0!=strstr(peers[j].ip,currentIP)){printf("Duplicated Connection!\n");return 0; } 
                        }
                        printf("selectserver: new connection from %s on "
                               "socket %d\n",currentIP,
                               newfd);
                        //printf("The IP is %s\n",remoteIP);
                        for(j=0;j<5;j++){
                            if(peers[j].used==0){
                                peers[j].used = 1;
                                peers[j].sockNum = newfd;
                                strcpy(peers[j].ip,remoteIP);
                                break;

                            }                        
                        }
                                                
                        /*  if ((recv(newfd,peerName, 127, 0)) == -1) {
                         perror("recv");
                         exit(1);
                         }
                         printf("PeerName %s\n",peerName); */
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, bufRecv, sizeof(bufRecv), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            for(j=0;j<5;j++){
                                if(peers[j].sockNum == i){
                                    peers[j].used = 0;
                                    break;
                                }
                            }
                        } else {
                            perror("recv");
                        }
                        
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        
                        //  printf("The Msg Got is %s\n",bufRecv);
                        if(0!=strstr(bufRecv,"UPLOAD")) {  //we Got an Uploda Request
                            /* If we got an UPLOAD request, we will first send a msg similiar to UPLOAD NAME 1.txt SIZE 512,then send the real file */
                            transferstate = 1;
                            mkdir("downLoad",S_IREAD | S_IWRITE | S_IEXEC);
                            long int fileSize;
                            char fileName[128];
                            char tempFileName[64];
                            strcpy(fileName,"downLoad/");
                            //			   sprintf(msgSend,"UPLOAD NAME %s SIZE %ld",fileName,fileSize);
                            sscanf(bufRecv,"%*s %*s %s %*s %ld",tempFileName,&fileSize); 
                            strcat(fileName,tempFileName);
                            /*for(j=0;j<fileSize/256;j++){
                             memset(bufRecv,0,sizeof(bufRecv));
                             if(recv(i,bufRecv,sizeof(bufRecv),0)<=0){perror("Recv");exit(4);}
                             FILE* pFileWrite;
                             pFileWrite = fopen("downLoad/downoad.txt","a");
                             
                             }*/
                            FILE* pFileWrite;
                            pFileWrite = fopen(fileName,"wb+");
                            //pFileWrite = fopen("downLoad/1.txt","w");
                            recvFile(i,pFileWrite,fileSize);
                            transferstate = 0;
                        } else if(0!=strstr(bufRecv,"DOWNLOAD")){  //We got an download Message
                            /* If we got an download message, we will first send a message with the format " LENGTH 512 ", then send the file 
                             
                             */
                            transferstate = 1;
                            
                            char fileName[128];
                            char msgRecv[128];
                            char msgSend[128];
                            long int fileSizeSend;
                            FILE* pFileRead;
                            sscanf(bufRecv,"%*s %s",fileName);
                            //printf("The File To Send is %s\n",fileName);
                            if(!strcmp(strstr(fileName,"."),".txt")) pFileRead = fopen(fileName,"r");
                            else{pFileRead = fopen(fileName,"rb"); }
                            if(NULL == pFileRead){perror("File Not Exist\n");exit(4);}
                            fseek(pFileRead,0,SEEK_END);
                            fileSizeSend = ftell(pFileRead);
                            fseek(pFileRead,0,SEEK_SET);
                            //printf("Sender Side %ld\n",fileSizeSend);
                            sprintf(msgSend,"LENGTH %ld",fileSizeSend);
                            
                            fflush(stdout);
                            if(send(i,msgSend,sizeof(msgSend),0)==-1){
                                perror("send");
                                exit(4);
                            } else{
                                sendFile(i,pFileRead);  
                            }
                            
                            
                            
                            
                        }
                        
                        // we got some data from a client
                        /*for(j = 0; j <= fdmax; j++) {
                         // send to everyone!
                         if (FD_ISSET(j, &master)) {
                         // except the listener and ourselves
                         if (j != listener && j != i) {
                         if (send(j, buf, nbytes, 0) == -1) {
                         perror("send");
                         }
                         }
                         }
                         }*/
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}
