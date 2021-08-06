#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include<arpa/inet.h>
#include <signal.h>

#define ERROR     -1			//returns -1 on error as message
#define BUFFER    512      //this is max size of input and output buffer used to store send and recieve data
//#define LISTENING_PORT 
#define MAX_CLIENTS    4     //defines maximum number of clients for listening


main(int argc, char **argv){  //IP and port mentioned

	int sock; // sock is socket desriptor for connecting to remote server 
	struct sockaddr_in remote_server; // contains IP and port no of remote server
	char input[BUFFER];  //user input stored 
	char output[BUFFER]; //recd from remote server
	int len;//to measure length of recieved input stram on TCP
	char *keyword_to_be_send; // variable to store temporary values
	int choice;//to take user input
	int LISTENING_PORT;
	LISTENING_PORT=(atoi(argv[3]));//take arguement three as peer/ own listening port

	//variables declared for fetch operation
	char file_fetch[BUFFER];//store file name keyword to be fetched
	char file_save[BUFFER];
	char peer_ip[BUFFER];//store IP address of the peer for connection
	char peer_port[BUFFER];//store port no of the peer for fetch
	struct sockaddr_in peer_connect; // contains IP and port no of desired peer for fetch
	int peer_sock; // socket descriptor for peer during fetch

	// variables for chatting
	char message[BUFFER];

	//variables for acting as server
	int listen_sock; // socket descriptor for listening to incoming connections
	struct sockaddr_in server; //server structure when peer acting as server
	struct sockaddr_in client; //structure for peer acting as server to bind to particular incoming peer
	int sockaddr_len=sizeof (struct sockaddr_in);	
	int child;//variable to store process id of process created after fork

	//variables for select system call
	fd_set master; // this is master file desriptor
	fd_set read_fd; // for select
	
	//----------------------------------------------------------------------------------------------

	//client takes three arguements SERVER IP ADDRESS,SERVER PORT WITH PEER LISTENING PORT
	if (argc < 3){    // check whether port number provided or not
		fprintf(stderr, "ERROR, ENTER SERVER IP ADDRESS AND PORT WITH YOUR LISTENING PORT\n");
		exit(-1);
	}

	//for connecting with server for publishing and search files
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR){ 
		perror("socket");  // error checking the socket
		exit(-1);  
	} 
	  
	remote_server.sin_family = AF_INET; // family
	remote_server.sin_port =htons(atoi(argv[2])); // Port No and htons to convert from host to network byte order. atoi to convert asci to 		integer
	remote_server.sin_addr.s_addr = inet_addr(argv[1]);//IP addr in ACSI form to network byte order converted using inet
	bzero(&remote_server.sin_zero, 8); //padding zeros
	
	if((connect(sock, (struct sockaddr *)&remote_server,sizeof(struct sockaddr_in)))  == ERROR){ //pointer casted to sockaddr*
		perror("connect");
		exit(-1);
	}
	//setting up own port for listening incoming connections for fetch
	//initialising 
	
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR){  // creating socket
		perror("socket");  // error while checking the socket
		exit(-1);  
	} 


	/*peer as server (to implement p2p concept)*/ 
	server.sin_family = AF_INET; // protocol family
	server.sin_port =htons(LISTENING_PORT); // Port No and htons to convert from host to network byte order. 
	server.sin_addr.s_addr = INADDR_ANY;//INADDR_ANY means server will bind to all netwrok interfaces on machine for given port no
	bzero(&server.sin_zero, 8); //padding zeros

	/*binding the listening socket */
	if((bind(listen_sock, (struct sockaddr *)&server, sockaddr_len)) == ERROR){ //pointer casted to sockaddr*
		perror("bind");
		exit(-1);
	}

	/*listen the incoming connections */
	if((listen(listen_sock, MAX_CLIENTS)) == ERROR){ // listen for max connections
		perror("listen");
		exit(-1);
	}

	//using select system call to handle multiple connections
	FD_ZERO(&master);// clear the set 
	FD_SET(listen_sock,&master) ; //adding our descriptor to the set
	int i;

	child=fork(); //create a background listen process for incoming connections
	
	if (!child) {
		while(1){
			read_fd = master; //waiting for incoming request
			// usign select() to handle multiple connections
			if(select(FD_SETSIZE,&read_fd,NULL,NULL,NULL)==-1){
				perror("select");
				return -1;
			}

			//handle multiple connections
			for (i = 0; i < FD_SETSIZE; ++i){
				if(FD_ISSET(i,&read_fd)){ //returns true if i in read_fd
					if(i==listen_sock){
						int new_peer_sock; //new socket descriptor for peer
						// accept takes pointer to variable containing len of struct
						if ((new_peer_sock= accept(listen_sock, (struct sockaddr *)&client, &sockaddr_len)) == ERROR) {
							perror("ACCEPT.Error accepting new connection");
							exit(-1);
						}

						else{
							FD_SET (new_peer_sock, &master); // add to master set --> Note: we use FD_CLR to remove it
							//printf("New peer connected from port no %d and IP %s\n", ntohs(client.sin_port),inet_ntoa(client.sin_addr));
						}
					}
					else{//handle data from a client
						bzero(input, BUFFER); 
						if((len = recv(i,input,BUFFER,0)) <= 0){//connection closed by client or error
							if(len == 0){//connection closed
								printf("Peer %d with IP address %s disconnected\n",i,inet_ntoa(client.sin_addr));
							}
							else{ //error
								perror("ERROR IN RECIEVE");
							}
							close(i);//closing this connection
							FD_CLR(i,&master);//remove from master set
						}

						else{
							printf("%s\n", input); //file name of file requested by other client

							//file read and transfer operation starts from here

							char* requested_file = input; // create file handler pointer for file Read operation

							FILE *file_request = fopen(requested_file, "r"); //opening the existing file
							
							if(file_request == NULL){ //If requested file not found at given path on given peer
								// fprintf(stderr, "ERROR : Opening requested file.REQUESTED FILE NOT FOUND \n");
								close(i);//closing this connection
								FD_CLR(i,&master);//remove from master set
							}

							else{
								bzero(output, BUFFER); 
								int file_request_send; //variable to store bytes recieved
								
								// read file and send bytes
								while((file_request_send = fread(output, sizeof(char), BUFFER, file_request)) > 0){
									if((send(i, output, file_request_send, 0)) < 0){ // error while transmiting file
										fprintf(stderr, "ERROR: Not able to send file");
										//exit(1);
									}

									bzero(output, BUFFER);
								}//close while loop for file read
								//fclose(file_request);
								close(i);//closing this connection
								FD_CLR(i,&master);//remove from master set
							} // close else of non error loop
					}//close else loop
				}//close else loop for handling data of existing connection
			}//close if statement of fd_isset
		
			}//close for loop for max file desriptor

		}// close while loop inside fork.peer will keep listening client till disconnected
		
		close(listen_sock); // close listening port
		exit(0);
	}//close if of fork system call

	//----------------------------------------------------------------------------------------------
	while(1)
	{
		//DISPLAY MENU FOR USER INPUTS
		printf("\nWELCOME. ENTER YOUR CHOICE\n");
		printf("1.PUBLISH FILE\n");
		printf("2.SEARCH FILE\n");
		printf("3.FETCH FILE\n");
        printf("4.SEND MESSAGE\n");
		printf("5.TERMINATE CONNECTION FROM SERVER\n");
		printf("6.EXIT\n");
		printf("Enter your choice : ");
		if(scanf("%d",&choice)<=0){
	    	printf("Enter only an integer from 1 to 5\n");
	    	kill(child,SIGKILL); // on exit, the created lsitening process to be killed
	    	exit(0);
			} 

		else{
	   		switch(choice){
				case 1:  //PUBLISH OPERATION
					
					keyword_to_be_send = "pub"; // keyword to be send to server so that server knows it is a publish operation

					send(sock, keyword_to_be_send, sizeof(keyword_to_be_send) ,0); // send input to server
					
					printf("Enter the file name with extension, Filepath and Port_No: ");
		   			//fgets(input,BUFFER,stdin); //take input from user
					scanf(" %[^\t\n]s",input); //recieve user input
					send(sock, input, strlen(input) ,0); // send input to server
					len = recv(sock, output, BUFFER, 0); // recieve confirmation message from server
					output[len] = '\0';
					printf("%s\n" , output); // display confirmation message
					bzero(output,BUFFER); // pad buffer with zeros

                 	break;

      		 	case 2:   //SEARCH OPERATION

					keyword_to_be_send = "sea"; // keyword to be send to server so that server knows it is a search operation

					send(sock, keyword_to_be_send, sizeof(keyword_to_be_send) ,0); // send input to server
					
					printf("Enter the file name to search:\t");	
					scanf(" %[^\t\n]s",input);
					send(sock, input, strlen(input) ,0); // send input keyword to server
					len = recv(sock, output, BUFFER, 0);
					output[len] = '\0';
					printf("%s\n" , output);
					bzero(output,BUFFER);  
								
					printf("Server searching...... Waiting for response\n");
					printf("1.Filename 2.Filepath 3.Port No        4.Peer IP\n"); //format of recieved output from server 
					while((len=recv(sock, output, BUFFER, 0))>0){
						output[len] = '\0'; // checking null for end of data
						printf("%s\n", output);
						bzero(output,BUFFER);
						}
					close(sock); // Disconnect from server
					printf("SEARCH COMPLETE!!! \n");
					printf("DISCONNECTED FROM SERVER. GO TO OPTION 3 FOR FETCH");
	                break;

        		case 3: //FETCH OPERATION

        			printf("Enter file path you want to download:(./upload_files/<filename.extn>):\t");
        			scanf(" %[^\t\n]s",file_fetch);
					printf("Enter file path you want to save file:(./download_files/<filename.extn>):\t");
        			scanf(" %[^\t\n]s",file_save);
        			printf("Enter peer IP address\t");
        			scanf(" %[^\t\n]s",peer_ip);
        			printf("Enter peer listening port number\t");
        			scanf(" %[^\t\n]s",peer_port);

        			//create socket to contact the desired peer
        			if ((peer_sock= socket(AF_INET, SOCK_STREAM, 0)) == ERROR){ 
						perror("socket"); 
						 // error checking the socket
						kill(child,SIGKILL); // on exit, the created lsitening process to be killed
						exit(-1);  
					} 
	  
					peer_connect.sin_family = AF_INET; // family
					peer_connect.sin_port =htons(atoi(peer_port)); // Port No and htons to convert from host to network byte order. atoi to convert asci to integer
					peer_connect.sin_addr.s_addr = inet_addr(peer_ip);//IP addr in ACSI form to network byte order converted using inet
					bzero(&peer_connect.sin_zero, 8); //padding zeros
					
					/*try to connect desired peer*/
					//pointer casted to sockaddr*
					if((connect(peer_sock, (struct sockaddr *)&peer_connect,sizeof(struct sockaddr_in)))  == ERROR){ 
						perror("connect");
						kill(child,SIGKILL); // on exit, the created lsitening process to be killed
						exit(-1);
					}
					
					//send file keyword with path to peer
					send(peer_sock, file_fetch, strlen(file_fetch) ,0); //send file keyword to peer
					printf("Recieving file from peer. Please wait \n"); // if file found on client/peer

					//file recieve starts from here
					FILE *file_saving = fopen(file_save, "w");
					if(file_saving == NULL){ //error creating file 
						printf("File %s cannot be created.\n", file_saving);
					}

					else{
						bzero(input,BUFFER);
		    			int file_fetch_size=0;
		    			int len_recd=0; 
		    			while((file_fetch_size = recv(peer_sock, input, BUFFER, 0)) > 0){ // recieve file sent by peer 
							len_recd = fwrite(input, sizeof(char),file_fetch_size,file_saving);

							if(len_recd < file_fetch_size){ //error while writing to file
								error("Error while writing file.Try again\n");
								kill(child,SIGKILL); // on exit, the created lsitening process to be killed
								exit(-1);
							}

							bzero(input,BUFFER);

							if(file_fetch_size == 0 || file_fetch_size != 512) { //error in recieve packet								
								break;
							}
		        		}

		        		if(file_fetch_size < 0){ //error in recieve
		      				error("Error in recieve\n");	  
							exit(1);
	            		}
		        			
		    	 		fclose(file_saving); //close opened file
						printf("FETCH COMPLETE");
						close(peer_sock); //close socket
					}	
					
                	break;
                
                case 4: // sending message
        			printf("Enter peer IP address\t");
        			scanf(" %[^\t\n]s",peer_ip);
        			printf("Enter peer listening port number\t");
        			scanf(" %[^\t\n]s",peer_port);

        			//create socket to contact the desired peer
        			if ((peer_sock= socket(AF_INET, SOCK_STREAM, 0)) == ERROR){ 
						perror("socket"); 
						 // error checking the socket
						kill(child,SIGKILL); // on exit, the created lsitening process to be killed
						exit(-1);  
					} 
	  
					peer_connect.sin_family = AF_INET; // family
					peer_connect.sin_port =htons(atoi(peer_port)); // Port No and htons to convert from host to network byte order. atoi to convert asci to integer
					peer_connect.sin_addr.s_addr = inet_addr(peer_ip);//IP addr in ACSI form to network byte order converted using inet
					bzero(&peer_connect.sin_zero, 8); //padding zeros
					
					/*try to connect desired peer*/
					//pointer casted to sockaddr*
					if((connect(peer_sock, (struct sockaddr *)&peer_connect,sizeof(struct sockaddr_in)))  == ERROR){ 
						perror("connect");
						kill(child,SIGKILL); // on exit, the created lsitening process to be killed
						exit(-1);
					}

					printf("Enter message:\t");
					scanf(" %[^\t\n]s",message);
					send(peer_sock, message, strlen(message) ,0);

					//message recieve starts from here
					bzero(message,BUFFER);
					int message_size=0;
					int len_recd=0; 
					while((message_size = recv(peer_sock, message, BUFFER, 0)) > 0){ // recieve file sent by peer 
						printf("%s", message);

						bzero(message,BUFFER);
						
						//error in recieve
						if(message_size == 0 || message_size != 512) { 								
							break;
						}
					}

					//error in recieve
					if(message_size < 0){ 
						error("Error in recieve\n");	  
						exit(1);
					}
					
					

					close(peer_sock); //close socket
						
					
                	break;


        		case 5:  //when client want to terminate connection with server  
					keyword_to_be_send="ter"; // keyword to be send to server so that server knows client wants to terminate connection to server
					send(sock, keyword_to_be_send, sizeof(keyword_to_be_send) ,0); // send input to server
					close(sock);
					printf("Connection terminated with server.\n");
					break;

       			case 6:    
					kill(child,SIGKILL); // on exit, the created lsitening process to be killed
					close(sock);
					return 0;

        		default:
					printf("Invalid option\n");
			} // terminates switch case
		
		} // terminates else statement
	} // terminates while loop 
		

	close(listen_sock);
	return (0);
}
