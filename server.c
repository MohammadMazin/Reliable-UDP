#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define SIZE 500
#define WIN_SIZE 5

typedef struct packet{
    char data[488];
}Packet;

typedef struct frame{
    int frame_kind; //ACK:0, SEQ:1 FIN:2
    int sq_no;
    int ack;
    Packet packet;
}Frame;


// void write_file(int sockfd, struct sockaddr_in addr)
// {


  
// }

int main()
{

  // Defining the IP and Port
  char* ip = "0.0.0.0";
  const int port = 8081;

  // Defining variables
  int server_sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[SIZE];
  int e;

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

  e = bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (e < 0)
  {
    perror("[ERROR] bind error");
    exit(1);
  }

	printf("[STARTING] UDP File Server started. \n");

	int sockfd = server_sockfd;
	struct sockaddr_in addr = client_addr;

	int n;
	socklen_t addr_size;

	int frame_id = 0;
	Frame frame_send;
	Frame frame_recv;
	int ack_recv = 1;
	Frame win[WIN_SIZE];
	bool rwin[WIN_SIZE];
	int ack_id = 0;
	int seq_count = 0;
	
	int winloopexpected = 0;	//expected window packet
	bool allreceived = false;	//flag to see all packets received
	int countcheck = 00;			//counter to see how many packets received
	
	//User defines filename
	char str1[20];
	printf("Enter filename of video to receive: ");
	scanf("%s", str1);
	//Write to file in binary
	FILE* fp = fp = fopen(str1, "wb");
	fseek(fp, 0, SEEK_SET);

	//loop for receiving and writing data
	while(1){
		//Reset all counters to default state
		countcheck = 0;
		for(int i=0;i<5;i++){
				rwin[WIN_SIZE]=false;
			}
		bool finish = false;
		allreceived = false;

		//receiving loop
		while (1){
				addr_size = sizeof(addr);
				n = recvfrom(sockfd, &frame_recv, sizeof(Frame), 0, (struct sockaddr*)&addr, &addr_size);	//receive packet from socket

				//if no data receievd, check if reciving window has packet in all of its index
				if(n==-1){
					for(int i=0;i<5;i++){
						if(rwin[i]==false){
							winloopexpected=i;
							allreceived = false;
							printf("Unexpected Packet c:%d\tsNo:%d\tE:%d\n",countcheck,frame_recv.sq_no,winloopexpected);
							//tell client which packet is needed
							frame_send.ack = winloopexpected;
							frame_send.frame_kind = 0;
							sendto(sockfd, &frame_send, sizeof(Frame), 0, (struct sockaddr*)&addr, sizeof(addr));
							break;
						}			
					}
				}

				//recieved frame is valid and not all packets are received
				if(frame_recv.frame_kind == 1 && countcheck!=5){
					
					//write packet data to received window if that index is expty
					if(rwin[frame_recv.sq_no]==false){
						printf("Packet number:%d\t\t",frame_id++);
						rwin[frame_recv.sq_no]=true;
						printf("Received packet %d\n",frame_recv.ack);
						win[frame_recv.sq_no].frame_kind = frame_recv.frame_kind;	
						for(int j=0;j<488;j++){
							win[frame_recv.sq_no].packet.data[j]=frame_recv.packet.data[j];
						}
						countcheck++;
					}	
				}

				//if last packet of file from client has been received
				if(frame_recv.frame_kind==2){
					if(rwin[frame_recv.sq_no]==false){
							printf("Packet number:%d\t\t",frame_id++);
							rwin[frame_recv.sq_no]=true;
							printf("Received packet %d\n",frame_recv.ack);
							for(int j=0;j<488;j++){
								win[frame_recv.sq_no].packet.data[j]=frame_recv.packet.data[j];
							}
							//mark flags to indicate all data received
							finish=true;
							allreceived=true;
							countcheck++;
							break;
					}	
				}	

				if(countcheck == 5){
					allreceived = true;
					break;
				}

			}
		
		//write data in output file if all packets of window received it's not the last window 
		if(allreceived && countcheck==5 && finish==false){
				printf("Received all Packets of window \n");
				//write data
				for(int i=0; i<5; i++){
					fwrite(win[i].packet.data,1,488,fp);
					bzero(win[i].packet.data, 488);		//reset data in received window
					rwin[i]=false;
				}
				
				frame_send.ack = 5; //flag indicating client that next Packet Needed
				frame_send.frame_kind = 0;
				bzero(frame_send.packet.data, 488);
				n = sendto(sockfd, &frame_send, sizeof(Frame), 0, (struct sockaddr*)&addr, sizeof(addr));
				continue;
			}

		//write data in output file if all packets of window received and no more data received 
		if(allreceived && finish){
			printf("allRec and Fin\n");
				
				frame_send.ack = 5; //ACK Of next Packet Needed
				frame_send.frame_kind = 0;
				bzero(frame_send.packet.data, 488);		

				for(int i=0; i<countcheck; i++){
					fwrite(win[i].packet.data,1,488,fp);
					bzero(win[i].packet.data, 488);		//reset data in received window
					rwin[i]=false;
				}

				//tell client all data receievd
				n = sendto(sockfd, &frame_send, sizeof(Frame), 0, (struct sockaddr*)&addr, sizeof(addr));
				fclose(fp);
				printf("[SUCCESS] Data transfer complete.\n");
				printf("[CLOSING] Closing the server.\n");
				close(server_sockfd);
				return 0;
			
		}
		
	}


  return 0;

}

