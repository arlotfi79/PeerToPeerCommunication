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
#include <time.h>

#define ERROR     -1  // defines error message
#define MAX_CLIENTS    4 //defines maximum number of clients that can connect simultaneously
#define MAX_BUFFER    512 //used to set max size of buffer for send recieve data 

int add_IP(char*); // function to add IP to the list
int update_IPlist(char *);//function to update IP_list once client disconnected
time_t current_time; // variable to store current time


main(int argc, char **argv){ 

	int sock; // sock is socket desriptor for server 
	int newClient; // socket descriptor for newClient client
	struct sockaddr_in server; //server structure 
	struct sockaddr_in client; //structure for server to bind to particular machine
	int sockaddr_len = sizeof (struct sockaddr_in);	//stores length of socket address
	


	//variables for publish operation
	char buffer[MAX_BUFFER]; // Receiver buffer; 
	char file_name[MAX_BUFFER];//Buffer to store filename,path and port recieved from peer
	char *peer_ip;//variable to store IP address of peer

	//varriable for search operation
	char user_key[MAX_BUFFER];//file keyword for search by user
	int len;// variable to measure MAX_BUFFER of incoming stream from user

	int child;// to manage child process
//----------------------------------------------------------------------------------------------
	if (argc < 2){    // check whether port number provided or not
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}


	/*get socket descriptor */
	if ((sock= socket(AF_INET, SOCK_STREAM, 0)) == ERROR){
		perror("server socket error: ");  // error checking the socket
		exit(-1);  
	} 
	 
	/*server structure */ 
	server.sin_family = AF_INET; // protocol family
	server.sin_port =htons(atoi(argv[1])); // Port No and htons to convert from host to network byte order. atoi to convert asci to integer
	server.sin_addr.s_addr = INADDR_ANY;//INADDR_ANY means server will bind to all netwrok interfaces on machine for given port no
	bzero(&server.sin_zero, 8); //padding zeros
	
	/*binding the socket */
	if((bind(sock, (struct sockaddr *)&server, sockaddr_len)) == ERROR){ //pointer casted to sockaddr*
		perror("bind");
		exit(-1);
	}

	/*listen the incoming connections */
	if((listen(sock, MAX_CLIENTS)) == ERROR){ // listen for max connections
		perror("listen");
		exit(-1);
	}

	while(1){
		// accept takes pointer to variable containing len of struct
		if ((newClient = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == ERROR){ 
			perror("ACCEPT.Error accepting newClient connection");
			exit(-1);
		}

		child = fork(); //creates separate process for each client at server

		if (!child){ // For multiple connections.this is the child process
            close(sock); // close the socket for other connections
            		
			printf("New client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
			peer_ip = inet_ntoa(client.sin_addr);
			add_IP(peer_ip); //adding Client IP into IP List
			
			while(1){
			len = recv(newClient, buffer , MAX_BUFFER, 0);
			buffer[len] = '\0';
			printf("%s\n",buffer);

			//conenctionError checking
				if(len<=0){//connection closed by client or error
					if(len==0){//connection closed
						printf("Peer %s disconnected\n",inet_ntoa(client.sin_addr));
						update_IPlist(peer_ip);
					} else { //error
						perror("ERROR IN RECIEVE");
					}
				close(newClient);//closing this connection
				exit (0);
				}//close if loop

			//PUBLISH OPERATION
			if(buffer[0]=='p' && buffer[1]=='u' && buffer[2]=='b'){ // check if user wants to publish a file

				/*Recieve publish files from peer */
				
				char* fileinfo = "filelist.txt";
				FILE *filedet = fopen(fileinfo, "a+");

				if(filedet==NULL){ // if unable to open file
					printf("ERROR. Unable to open File");
					return 1;  
				}   

				//adding publised file details to publish list
				else{
					fwrite("\n", sizeof(char), 1, filedet);			
						
					len = recv(newClient, file_name, MAX_BUFFER, 0); //recv file details from client, data is pointer where data recd will be stored
					fwrite(&file_name, len,1, filedet);
					char Report[] = "File published";
					send(newClient,Report,sizeof(Report),0);		

					fwrite("\t",sizeof(char),1, filedet);
					
					peer_ip = inet_ntoa(client.sin_addr); // return the IP
					
					fwrite(peer_ip,1, strlen(peer_ip), filedet);		//adding peer IP address to given file	
					fclose(filedet);
					printf("%s\n","FILE PUBLISHED");
				}
			}// close if pub check loop

			/*SEARCH OPERATION*/
			else if(buffer[0]=='s' && buffer[1]=='e' && buffer[2]=='a'){ //check keyword for search sent by client
			
				bzero(buffer,MAX_BUFFER); // clearing the buffer

				len = recv(newClient, user_key, MAX_BUFFER, 0); //recv keyword from client to search
				char Report3[] = "Keyword recieved"; 
				send(newClient,Report3,sizeof(Report3),0);
				user_key[len] = '\0';
				

				FILE *file_list = fopen("filelist.txt", "r");
				FILE *search_list = fopen("searchresult.txt", "a+");
				if(file_list == NULL) {
					fprintf(stderr, "ERROR while opening file on server.");
					exit(1);
				}
				if(search_list == NULL) {
					fprintf(stderr, "ERROR while opening file on server.");
					exit(1);
				}

				char line[50];
				int flag = 0;
				while(fgets(line, 512, file_list) != NULL) {
					if((strstr(line, user_key)) != NULL) {
						fwrite(line,1, strlen(line), search_list);
						fwrite("\n", sizeof(char), 1, search_list);
						flag = 1;
						break;
					}
				}
				
				if (flag == 0){
					char not_found[50] = "Sorry. Desired file not found";
					fwrite(not_found,1, strlen(not_found), search_list);
					fwrite("\n", sizeof(char), 1, search_list);
				}
				fclose(search_list);
				fclose(file_list);

				char* search_result = "searchresult.txt";
				char buffer[MAX_BUFFER]; 
				
				FILE *file_search = fopen(search_result, "r"); // open and read file conataining result
				if(file_search == NULL){
					fprintf(stderr, "ERROR while opening file on server.");
					exit(1);
				}

				bzero(buffer, MAX_BUFFER); 
				int file_search_send; 
				while((file_search_send = fread(buffer, sizeof(char), MAX_BUFFER, file_search))>0){ //send search result to peer
					len = send(newClient, buffer, file_search_send, 0);

					if(len < 0){
						fprintf(stderr, "ERROR: File not found");
						exit(1);
					}
					bzero(buffer, MAX_BUFFER);
				}
				fclose(file_search);
				char Reportsearch[] = "Search complete.You are disconnected from server now.Connect again for further actions"; 
				send(newClient,Reportsearch,sizeof(Reportsearch),0);
				
				printf("Search complete!!!!\n");
				printf("Client disconnected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
				peer_ip = inet_ntoa(client.sin_addr); // return the IP
				update_IPlist(peer_ip);
				
				close(newClient); // disconnect this client so that other users can connect server
				exit(0);
			}// close search condition

			//TERMINATE OPERATION:when user want to disconnect from server
			else if(buffer[0]=='t' && buffer[1]=='e' && buffer[2]=='r'){
				printf("Client disconnected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
				peer_ip = inet_ntoa(client.sin_addr); // return the IP
				update_IPlist(peer_ip);

				close(newClient);// close the connection
				exit(0);
			} //close terminate loop
		
		}// close while loop inside fork.server will keep listening client till disconnected
	} // close if loop for fork function
	close(newClient);
	}// close main while loop
	printf("Server shutting down \n");
	close(sock);
}


//--------------------------------------------------------------------------------------------
//updating Client IP in IP List
int update_IPlist(char *peer_ip){
	char* peerinfo = "peerIPlist.txt";
	FILE *peerdet = fopen(peerinfo, "a+");

	if(peerdet==NULL){ // if unable to open file
		printf("Unable to open IPList File.Error");
		return -1;  
	} else{
		fwrite("\n", sizeof(char), 1, peerdet);			
		// write details of IP of client
		fwrite(peer_ip,1, strlen(peer_ip), peerdet);		//adding peer IP address to given file	
		char update[] = "  disconnected from server at ";
		fwrite(update,1, strlen(update), peerdet);
		current_time = time(NULL);
		fwrite(ctime(&current_time),1, strlen(ctime(&current_time)), peerdet);
		fclose(peerdet);
	}
}

//----------------------------------------------------------------------------------------------
//adding Client IP in IP List
int add_IP(char *peer_ip)
{
	char* peerinfo = "peerIPlist.txt";
	FILE *peerdet = fopen(peerinfo, "a+");

	if(peerdet==NULL){ // if unable to open file
		printf("Unable to open IPList File.Error");
		return -1;  
	} else{
		fwrite("\n", sizeof(char), 1, peerdet);			
		// write details of IP of client
		fwrite(peer_ip,1, strlen(peer_ip), peerdet);		//adding peer IP address to given file	
		char update[] = "  connected to server at ";
		fwrite(update,1, strlen(update), peerdet);
		current_time = time(NULL);
		fwrite(ctime(&current_time),1, strlen(ctime(&current_time)), peerdet);
		fclose(peerdet);
	}
}
