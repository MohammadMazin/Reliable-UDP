#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define SIZE 500    //bytes of packet
#define WIN_SIZE 5  //size of window

typedef struct packet{
    char data[488];
}Packet;

typedef struct frame{
    int frame_kind; //ACK:0, SEQ:1 FIN:2
    int sq_no;
    int ack;
    Packet packet;
}Frame;


int main(void){

  // Defining the IP and Port
  char *ip = "127.0.0.1";
  const int port = 8081;

  // Defining variables
  int server_sockfd;
  struct sockaddr_in server_addr;
  char *filename = "vid.mp4";
  char str1[20];
  

  printf("Enter filename of video to send: ");
  scanf("%s", str1);
  
  FILE *fp = fopen(str1, "rb");
  fseek(fp, 0, SEEK_SET);
  

  // Creating a UDP socket
  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0)
  {
    perror("[ERROR] socket error");
    exit(1);
  }
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  // Reading the text file
  if (fp == NULL)
  {
    perror("[ERROR] reading the file");
    exit(1);
  }

  // Sending the file data to the server
  int sockfd = server_sockfd;
  struct sockaddr_in addr = server_addr;
  int n;
  char buffer[SIZE];
  
  Frame win[WIN_SIZE];

  int frame_id = 0;   //keeps count of packet numbers
  Frame frame_send;
  Frame frame_recv;
  int ack_recv = 1;

  socklen_t len;
  int ack_id = 0;
  int ack_count = 5;
  bool terminate = false;

  
  // Sending the data
  //send data until end of end of file is reached
  while (!feof(fp))
  {
    //loop that sends 5 packets at a time and then waits for ACKs from server
    for(int i=0; i<5; i++){
      
      //writing data and adding flags to packet
      fread(win[i].packet.data,1,488,fp);
      win[i].sq_no = i;
      win[i].ack = i;
      win[i].frame_kind = 1;

      //mark current packet as last packet of file
      if(feof(fp)) {   
        printf("Number of packets: %d\n",frame_id);
        win[i].frame_kind = 2;
      }
      
      //send packet to server
      n = sendto(sockfd, &win[i], sizeof(Frame), 0, (struct sockaddr*)&addr, sizeof(addr));
      
      if (n == -1)
      {
        perror("[ERROR] sending data to the server.");
        exit(1);
      }

      printf("Sent Packet :%d  %d\n",frame_id,win[i].sq_no);
      frame_id++;
      bzero(win[i].packet.data, 488);   //reset window data
 
    }
 
    //check for ACKs
    while(1){
      printf("Waiting for ack...\n");      
        //receive ACKs from server
        n = recvfrom(sockfd, &frame_recv, sizeof(Frame), 0, (struct sockaddr*)&addr, &len);
        
        //tells client that all packets have been received by server in correct order
        if(frame_recv.frame_kind == 0 && frame_recv.ack==5){
        printf("ACK received %d---%d\n",n,frame_recv.ack);
          break;
        }
        //tells client which packet has not been recieved
        else if (frame_recv.frame_kind == 0 && frame_recv.ack!=5){
          printf("Not received correct ACK\tFrame-Kind: %d\t\tFrame-ACK: %d\n",frame_recv.frame_kind, frame_recv.ack);
          //Restransmit packet
          sendto(sockfd, &win[frame_recv.ack], sizeof(Frame), 0, (struct sockaddr*)&addr, sizeof(addr));
          
        }
           
    }

  }
  fclose(fp);

  printf("[SUCCESS] Data transfer complete.\n");
  printf("[CLOSING] Disconnecting from the server.\n");

  close(server_sockfd);

  return 0;
}

