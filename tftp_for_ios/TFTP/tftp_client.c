/*-------------------------------------------------------------------
*function   :tftp client test
*date       :2017-05-17
*history    :
*------------------------------------------------------------------*/
#include "tftp_client.h"
#include "tftpx.h"

#define LINE_BUF_SIZE 1024

// Socket fd this client use.
static int sock;

// Server address.
struct sockaddr_in server;
socklen_t addr_len;
int blocksize = DATA_SIZE;

#define FILE_PATH_LEN       128
typedef struct _writed_file_info
{
    char file_name[FILE_PATH_LEN];
    char check[2];
}writed_file_info_t;

static writed_file_info_t writed_file_info_last;  

void help()
{
	printf("Usage: cmd  arg0[,arg1,arg2...]\n");
    
	printf("  -Directory listing:\n");
	printf("    list path\n");
	printf("  -Download a file from the server:\n");
	printf("    get remote_file[ local_file]\n");
	printf("  -Upload a file to the server:\n");
	printf("    put filename\n");
	printf("  -Set blocksize:\n");
	printf("    blocksize size\n");

    printf("  -Process start:\n");
	printf("    process_start process_name \n");

    printf("  -Process stop:\n");
	printf("    process_stop process_name \n");
    
    printf("  -Process retart:\n");
	printf("    process_restart process_name \n");
    
    printf("  -Check file:\n");
	printf("    check_file file_name \n");   
    
	printf("  -Quit this programm:\n");      
	printf("    quit\n");
    
}

//int main(int argc, char **argv)
//{
//	char cmd_line[LINE_BUF_SIZE];
//	char *buf;
//	char *arg;
//	int i;
//	char *local_file;
//	
//	int done = 0;	// Server exit.
//	char *server_ip;
//	unsigned short port = SERVER_PORT;
//
//	addr_len = sizeof(struct sockaddr_in);	
//	
//	if(argc < 2){
//		printf("Usage: %s server_ip [server_port]\n", argv[0]);
//		printf("    server_port - default 10220\n");
//		return 0;
//	}
//	help();
//	
//	server_ip = argv[1];
//	if(argc > 2){
//		port = (unsigned short)atoi(argv[2]);
//	}
//	printf("Connect to server at %s:%d", server_ip, port);
//	
//	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
//		printf("Server socket could not be created.\n");
//		return 0;
//	}
//	
//	// Initialize server address
//	server.sin_family = AF_INET;
//	server.sin_port = htons(port);
//	inet_pton(AF_INET, server_ip, &(server.sin_addr.s_addr));
//	
//	// Command line interface.
//	while(1){
//		printf(">> ");
//		memset(cmd_line, 0, LINE_BUF_SIZE);
//		buf = fgets(cmd_line, LINE_BUF_SIZE, stdin);
//		if(buf == NULL){
//			printf("\nBye.\n");
//			return 0;
//		}
//		
//		arg = strtok (buf, " \t\n");
//		if(arg == NULL){
//			continue;
//		}
//		
//		if(strcmp(arg, "list") == 0){
//			arg = strtok (NULL, " \t\n");
//			if(arg == NULL){
//				printf("Error: missing arguments\n");
//			}else{
//				do_list(sock, arg);
//			}
//		}else if(strcmp(arg, "get") == 0){
//			arg = strtok (NULL, " \t\n");
//			local_file = strtok (NULL, " \t\n");
//			if(arg == NULL){
//				printf("Error: missing arguments\n");
//			}else{
//				if(local_file == NULL){
//					local_file = arg;
//				}
//				do_get(arg, local_file);
//			}
//		}else if(strcmp(arg, "put") == 0){
//			arg = strtok (NULL, " \t\n");
//			if(arg == NULL){
//				printf("Error: missing arguments\n");
//			}else{
//				do_put(arg);
//			}
//		}else if(strcmp(arg, "blocksize") == 0){
//			arg = strtok (NULL, " \t\n");
//			if(arg == NULL){
//				printf("Error: missing arguments\n");
//			}else{
//				int blk = atoi(arg);
//				if(blk > 0 && blk <= DATA_SIZE){
//					blocksize = blk;
//				}else{
//					printf("Error: blocksize should be > 0 && <= DATA_SIZE\n");
//				}
//			}
//		}else if(strcmp(arg, "quit") == 0){
//		
//			break;
//            
//        }else if(strcmp(arg, "process_start")==0){
//    
//			arg = strtok (NULL, " \t\n");
//			if(arg == NULL){
//				printf("Error: missing arguments\n");
//			}else{
//				do_start_process(arg);
//			}
//
//        }else if(strcmp(arg, "process_stop")==0){
//            
//            arg = strtok (NULL, " \t\n");
//            if(arg == NULL){
//                printf("Error: missing arguments\n");
//            }else{
//                do_stop_process(arg);
//            }
//
//        }else if(strcmp(arg, "process_restart")==0){
//            
//            arg = strtok (NULL, " \t\n");
//            if(arg == NULL){
//                printf("Error: missing arguments\n");
//            }else{
//                do_restart_process(arg);
//            }
//
//        }else if(strcmp(arg, "check_file")==0){
//            
//            arg = strtok (NULL, " \t\n");
//            if(arg == NULL){
//                printf("Error: missing arguments\n");
//            }else{
//                do_check_file(arg);
//            }
//
//        }else{
//			printf("Unknow command.\n");
//		}
//        
//	}
//	return 0;
//}

int tftp_do_init(int server_port,char *server_ips)
{
//    unsigned short port = server_port;
//    char *server_ip = server_ips;
//    
//    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
//        printf("Server socket could not be created.\n");
//        return -1;
//    }
//
//    server.sin_family = AF_INET;
//    server.sin_port = htons(port);
//    inet_pton(AF_INET, server_ip, &(server.sin_addr.s_addr));
    char *server_ip = server_ips;
    unsigned short port = server_port;

    addr_len = sizeof(struct sockaddr_in);

    printf("Connect to server at %s , port:%d. \n", server_ip, port);

    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printf("Server socket could not be created.\n");
        return 0;
    }

    // Initialize server address
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &(server.sin_addr.s_addr));

    
    return 0;
}

// Download a file from the server.
void tftp_do_get(char *remote_file, char *local_file)
{
	struct tftpx_packet snd_packet, rcv_packet;
	int next_block = 1;
	int recv_n;
	int total_bytes = 0;
	struct tftpx_packet ack;	
	struct sockaddr_in sender;
		
	int r_size = 0;
	int time_wait_data;
	ushort block = 1;
	
	// Send request.
	snd_packet.cmd = htons(CMD_RRQ);
	sprintf(snd_packet.filename, "%s%c%s%c%d%c", remote_file, 0, "octet", 0, blocksize, 0);
	sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
	
	FILE *fp = fopen(local_file, "w");
	if(fp == NULL){
		printf("Create file \"%s\" error.\n", local_file);
		return;
	}
	
	// Receive data.
	snd_packet.cmd = htons(CMD_ACK);
	do{
		for(time_wait_data = 0; time_wait_data < PKT_RCV_TIMEOUT * PKT_MAX_RXMT; time_wait_data += 10000){
			// Try receive(Nonblock receive).
			r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
					(struct sockaddr *)&sender,
					&addr_len);
			if(r_size > 0 && r_size < 4){
				printf("Bad packet: r_size=%d\n", r_size);
			}
			if(r_size >= 4 && rcv_packet.cmd == htons(CMD_DATA) && rcv_packet.block == htons(block)){
				printf("DATA: block=%d, data_size=%d\n", ntohs(rcv_packet.block), r_size - 4);
				// Send ACK.
				snd_packet.block = rcv_packet.block;
				sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&sender, addr_len);
				fwrite(rcv_packet.data, 1, r_size - 4, fp);
				break;
			}
			usleep(10000);
		}
		if(time_wait_data >= PKT_RCV_TIMEOUT * PKT_MAX_RXMT){
			printf("Wait for DATA #%d timeout.\n", block);
			goto do_get_error;
		}
		block ++;
	}while(r_size == blocksize + 4);
	//printf("\nReceived %d bytes.\n", total_bytes);
	
do_get_error:
	fclose(fp);
}

typedef void(*CALLBACKFUNC)(int progress);

// Upload a file to the server.
void tftp_do_put(char *filename,char *filePath,void(*tftp_progress)(int progress),void(*tftp_complete)(char *error))
{
	struct sockaddr_in sender;
	struct tftpx_packet rcv_packet, snd_packet;
	int r_size = 0;
	int time_wait_ack;

    unsigned char check_1 = 0;
    unsigned char check_2 = 0; 
    unsigned int i;
    
    tftp_progress(0);
    
	// Send request and wait for ACK#0.
	snd_packet.cmd = htons(CMD_WRQ);
    char *pBuf = (char *)(&snd_packet.code);
    
    sprintf(pBuf, "%s%c%s%c%d%c", filename, 0, "octet", 0, blocksize, 0);
//    sprintf(pBuf, "%s%c%s%c%d%c", filename, 0, "octet", 0, blocksize, 0);
	int result = sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
    if (result <= 0) {
        tftp_complete("request faile");
        return;
    }
	for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
		// Try receive(Nonblock receive).
        r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
                          (struct sockaddr *)&sender,
                          &addr_len);
		if(r_size > 0 && r_size < 4){
			printf("Bad packet: r_size=%d\n", r_size);
		}
		if(r_size >= 4 && rcv_packet.cmd == htons(CMD_ACK) && rcv_packet.block == htons(0)){
			break;
		}
		usleep(20000);
	}
	if(time_wait_ack >= PKT_RCV_TIMEOUT){
		printf("Could not receive from server.\n");
        tftp_complete("Could not receive from server.");
		return;
	}
    
	FILE *fp = fopen(filePath, "r");
	if(fp == NULL){
		printf("File not exists!\n");
        tftp_complete("File not exists!");
		return;
	}
	
	int s_size = 0;
	int rxmt;
	ushort block = 1;
	snd_packet.cmd = htons(CMD_DATA);

    check_1 = 0;
    check_2 = 0; 
    
	// Send data.
    do{
        memset(snd_packet.data, 0, sizeof(snd_packet.data));
        snd_packet.block = htons(block);
        s_size = fread(snd_packet.data, 1, blocksize, fp);
        printf("read file size: %d \n", s_size);
        
        //cal check data
        for(i=0;i<s_size;i++){
            check_1 = check_1 + snd_packet.data[i];
            check_2 = check_2 ^ snd_packet.data[i];
        }
        
        for(rxmt = 0; rxmt < PKT_MAX_RXMT; rxmt ++){
            sendto(sock, &snd_packet, s_size + 4, 0, (struct sockaddr*)&sender, addr_len);
            printf("Send %d\n", block);
            
            // Wait for ACK.
            for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
                // Try receive(Nonblock receive).
                r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
                                  (struct sockaddr *)&sender,
                                  &addr_len);
                if(r_size > 0 && r_size < 4){
                    printf("Bad packet: r_size=%d\n", r_size);
                }
                if(r_size >= 4 && rcv_packet.cmd == htons(CMD_ACK) && rcv_packet.block == htons(block)){
                    printf("send ----- 1 \n");
                    break;
                }
                usleep(20000);
            }
            
            if(time_wait_ack < PKT_RCV_TIMEOUT){
                printf("send ----- 2 \n");
                // Send success.
                int currentSend;
                if (s_size == DATA_SIZE) {
                    currentSend = DATA_SIZE * block;
                }else{
                    currentSend = DATA_SIZE * (block-1)+s_size;
                }
                printf("send count %i \n",currentSend);
                tftp_progress(currentSend);
                break;
            }else{
                printf("send ----- 3 \n");
                // Retransmission.
                continue;
            }
        }
        if(rxmt >= PKT_MAX_RXMT){
            printf("Could not receive from server.\n");
            tftp_complete("Could not receive from server.");
            return;
        }
        
        block ++;
    }while(s_size == blocksize);
    //
    writed_file_info_last.check[0] = check_1;
    writed_file_info_last.check[1] = check_2;
    strcat(writed_file_info_last.file_name, filePath);
    printf("[tftpX]file check,%s, %d, %d \n",writed_file_info_last.file_name, writed_file_info_last.check[0], writed_file_info_last.check[1]);
// 上传完成
	printf("\nSend file end.\n");
    tftp_complete("success");
do_put_error:
	fclose(fp);
	
	return;
}

// Directory listing.
void tftp_do_list(int sock, char *dir)
{
	struct tftpx_packet packet;	
	int next_block = 1;
	int recv_n;
	struct tftpx_packet ack;	
	struct sockaddr_in sender;
	
	ack.cmd = htons(CMD_ACK);
	
	int r_size = 0;
	int time_wait_data;
	ushort block = 1;
	
	// Send request.
	packet.cmd = htons(CMD_LIST);
	strcpy(packet.data, dir);
	sendto(sock, &packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
	
	printf("type\tsize\tname\n");
	printf("-------------------------------------------------\n");
	
	// Receive data.
	do{
		for(time_wait_data = 0; time_wait_data < PKT_RCV_TIMEOUT * PKT_MAX_RXMT; time_wait_data += 20000){
			// Try receive(Nonblock receive).
			r_size = recvfrom(sock, &packet, sizeof(packet), MSG_DONTWAIT,
					(struct sockaddr *)&sender,
					&addr_len);
			if(r_size > 0 && r_size < 4){
				printf("Bad packet: r_size=%d\n", r_size);
			}
			if(r_size >= 4 && packet.cmd == htons(CMD_DATA) && packet.block == htons(block)){
				block ++;
				ack.block = packet.block;
				sendto(sock, &ack, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&sender, addr_len);
				fwrite(packet.data, 1, r_size - 4, stdout);
				break;
			}
			usleep(20000);
		}
		if(time_wait_data >= PKT_RCV_TIMEOUT * PKT_MAX_RXMT){
			printf("Wait for DATA #%d timeout.\n", block);
			return;
		}
	}while(r_size == blocksize + 4);
}

//optcode(2byte) + filename(\0,node:not path) + mode(octet/netascii \0)
void tftp_do_start_process(char *filename)
{
	struct sockaddr_in sender;
	struct tftpx_packet rcv_packet, snd_packet;
	int r_size = 0;
	int time_wait_ack;
	
	// Send request and wait for ACK#0.
	snd_packet.cmd = htons(CMD_PROCESS_START);
	sprintf(snd_packet.filename, "%s%c%s%c", filename, 0, "octet", 0);
    
	sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);	
    
	for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
		// Try receive(Nonblock receive).
		r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
				(struct sockaddr *)&sender,
				&addr_len);
		if(r_size > 0 ){
			printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
		}

		usleep(20000);
	}
    
	if(time_wait_ack >= PKT_RCV_TIMEOUT){
		printf("do_start_process fail! \n");
	}
   
	return;
}

//optcode(2byte) + filename(\0,node:not path) + mode(octet/netascii \0)
void tftp_do_stop_process(char *filename)
{
	struct sockaddr_in sender;
	struct tftpx_packet rcv_packet, snd_packet;
	int r_size = 0;
	int time_wait_ack;
	
	// Send request and wait for ACK#0.
	snd_packet.cmd = htons(CMD_PROCESS_STOP);
	sprintf(snd_packet.filename, "%s%c%s%c", filename, 0, "octet", 0);
    
	sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);	
    
	for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
		// Try receive(Nonblock receive).
		r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
				(struct sockaddr *)&sender,
				&addr_len);
		if(r_size > 0 ){
			printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
		}

		usleep(20000);
	}
    
	if(time_wait_ack >= PKT_RCV_TIMEOUT){
		printf("do_stop_process fail! \n");
		return;
	}
   
	return;
}

//optcode(2byte) + filename(\0,node:not path) + mode(octet/netascii \0)
void tftp_do_restart_process(char *filename)
{
	struct sockaddr_in sender;
	struct tftpx_packet rcv_packet, snd_packet;
	int r_size = 0;
	int time_wait_ack;
	
	// Send request and wait for ACK#0.
	snd_packet.cmd = htons(CMD_PROCESS_RESTART);
	sprintf(snd_packet.filename, "%s%c%s%c", filename, 0, "octet", 0);
    
	sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);	
    
	for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
		// Try receive(Nonblock receive).
		r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
				(struct sockaddr *)&sender,
				&addr_len);
		if(r_size > 0 ){
			printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
		}

		usleep(20000);
	}
    
	if(time_wait_ack >= PKT_RCV_TIMEOUT){
		printf("do_restart_process fail! \n");
		return;
	}
   
	return;
}

//optcode(2byte) + filename(\0,node:not path) + check(2byte)
int tftp_do_check_file(char *filename)
{
	struct sockaddr_in sender;
	struct tftpx_packet rcv_packet, snd_packet;
	int r_size = 0;
	int time_wait_ack;
	
	// Send request and wait for ACK#0.
	snd_packet.cmd = htons(CMD_FILE_CHECK);
    char *pBuf = (char *)(&snd_packet.code);

	sprintf(pBuf, "%s%c%c%c%c", writed_file_info_last.file_name, 0, writed_file_info_last.check[0], writed_file_info_last.check[1], 0);
    printf("check file,check0:%d, check1:%d  \n",writed_file_info_last.check[0], writed_file_info_last.check[1]);
    
	sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);	
    
	for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20000){
		// Try receive(Nonblock receive).
		r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
				(struct sockaddr *)&sender,
				&addr_len);
		if(r_size > 0 ){
			printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
		}
		usleep(20000);
	}
    
	if(time_wait_ack >= PKT_RCV_TIMEOUT){
		printf("do_restart_process fail! \n");
	}
    return ntohs(rcv_packet.code);
}

void do_reboot_app()
{
    struct sockaddr_in sender;
    struct tftpx_packet rcv_packet, snd_packet;
    int r_size = 0;
    int time_wait_ack;
    
    // Send request and wait for ACK#0.
    snd_packet.cmd = htons(CMD_PROCESS_REBOOT_APP);
    char *pBuf = (char *)(&snd_packet.code);
    sprintf(pBuf, "%c%s%c", 0, "octet", 0);
    sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
    
    for(time_wait_ack = 0; time_wait_ack < 30*1000*1000; time_wait_ack += 20000){
        // Try receive(Nonblock receive).
        r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
                          (struct sockaddr *)&sender,
                          &addr_len);
        if(r_size > 0 ){
            printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
        }
        
        usleep(20000);
    }
    
    if(time_wait_ack >= 30*1000*1000){
        printf("do_reboot_app timeout fail! \n");
        return;
    }
    
    return;
}

void do_reboot_sys()
{
    struct sockaddr_in sender;
    struct tftpx_packet rcv_packet, snd_packet;
    int r_size = 0;
    int time_wait_ack;
    
    // Send request and wait for ACK#0.
    snd_packet.cmd = htons(CMD_PROCESS_REBOOT_SYS);
    char *pBuf = (char *)(&snd_packet.code);
    sprintf(pBuf, "%c%s%c", 0, "octet", 0);
    sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
    
    for(time_wait_ack = 0; time_wait_ack < 5*1000*1000; time_wait_ack += 20000){
        // Try receive(Nonblock receive).
        r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
                          (struct sockaddr *)&sender,
                          &addr_len);
        if(r_size > 0 ){
            printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
        }
        
        usleep(20000);
    }
    
    if(time_wait_ack >= 5*1000*1000){
        printf("do_reboot_app timeout fail! \n");
        return;
    }
    
    return;
}

int do_upgrade()
{
    struct sockaddr_in sender;
    struct tftpx_packet rcv_packet, snd_packet;
    int r_size = 0;
    int time_wait_ack;
    
    // Send request and wait for ACK#0.
    snd_packet.cmd = htons(CMD_PROCESS_UPGRADE);
    char *pBuf = (char *)(&snd_packet.code);
    sprintf(pBuf, "%c%s%c", 0, "octet", 0);
    sendto(sock, &snd_packet, sizeof(struct tftpx_packet), 0, (struct sockaddr*)&server, addr_len);
    
    for(time_wait_ack = 0; time_wait_ack < 30*1000*1000; time_wait_ack += 20000){
        // Try receive(Nonblock receive).
        r_size = recvfrom(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT,
                          (struct sockaddr *)&sender,
                          &addr_len);
        if(r_size > 0 ){
            printf("Recv ack,r_size:%d, cmd:%d, code:%d \n", r_size,ntohs(rcv_packet.cmd),ntohs(rcv_packet.code));
            break;
        }
        
        usleep(20000);
    }
    
    if(time_wait_ack >= 30*1000*1000){
        printf("do_reboot_app timeout fail! \n");
        
    }
    
    return ntohs(rcv_packet.code);
}


