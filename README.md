# Peer to Peer Chatroom with File Transfer
This project was designed by [Professor Ali Hamzeh](https://scholar.google.com/citations?user=3AijUAwAAAAJ&hl=en) 
for the Principles of Programming course (Winter 2020).

## Dependencies
This project only runs on a linux Operating System or WSL (Windows Subsystem for Linux).

## Description
The goal of this project is to develop a P2P chatroom which has the following features:
 * Connection between peers should not disconnect, neither server runs into problem nor disconnects.
 * Peers should be able to transfer files (At least .txt files).

## Implementation
Directory details and overall workflow of the system is as follows:  

 * Server Directory
   * fileserver.c : This file creates a central server. File takes one commandline input arguments ie own listening port.
   Clients will use Server IP and port to connect this server for publish and search files.
   
   * filelist.txt :This file contains the list of files published by the peers. It provides details in the following format:  
     file_name &nbsp;&nbsp; file_path  &nbsp;&nbsp;  peer_port_number &nbsp;&nbsp;	peer_IP_address
   
   * peerIPList.txt : This file contains log of IP addresses of the all peers which has connected to the start of server.
   
   * searchresult: This file contains search results whether successful or not. It provides information about a peer 
     to the other peer when he/she wants to make a connection.  
   
   
 * Client Directory
   * fileclient.c : This file creates a peer. File takes three commandline input arguments server-ip, server port and own listening port.  
     Server IP and port will be used to connect the server and publish and search files.  
     Own listening port will be used to listen to incoming fetch requests from other peers.
   
   * upload_files and download_files folder : All files which client want to upload or download will be kept in these folders.
     Each client should have this folder. Else can result errors.  
   
   
 * How to run the Server
   1. Compile using -> gcc fileserver.c -o server
   2. Run using -> ./server <port no>  
   

 * How to run the Client
   1. Compile using -> gcc fileclient.c -o client
   2. Run using -> ./client <server IP address> <Server listening port no> <Own listening port no for peers>  
   
   
 * Follow commands on screen for file publish,search, fetch and p2p chat
   1. Publish a file   
      Format: Filename with extension, file path (if required), own listening port
   2. Search file  
      Format: Filename
   3. Fetch  
       Format:
      1. Filename (if in the same directory) or ./<folder_path>/<filename.extn>
      2. clientIP
      3. port No 
   4. P2P chat with other peers using IP_address and Port_no
   5. Terminate connection to server
   6. Exit
   