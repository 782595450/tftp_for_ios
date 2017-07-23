#ifndef TFTPX_H
#define TFTPX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>

#define SERVER_PORT 28

// Max request datagram size
#define MAX_REQUEST_SIZE 1024

// TFTPX_DATA_SIZE
#define DATA_SIZE 512
//
#define LIST_BUF_SIZE (DATA_SIZE * 8)

// Max packet retransmission.
#define PKT_MAX_RXMT 3

// usecond
#define PKT_SND_TIMEOUT 12*1000*1000
#define PKT_RCV_TIMEOUT 3*1000*1000

// usecond
#define PKT_TIME_INTERVAL 5*1000

#define PKT_RCV_RERTY_COUNT     300
#define PKT_RCV_RERTY_INTERVAL  20000 //time=20ms

struct tftpx_packet{
    ushort cmd;
    union{
        ushort code;
        ushort block;
        // For a RRQ and WRQ TFTP packet
        char filename[2];
    };
    char data[DATA_SIZE];
};

typedef struct {
    unsigned short opcode;
    unsigned short block;
}tftp_ack;

#define MAX_FILENAME	 255
#define MAX_MODE	     255

typedef struct {
    unsigned short opcode;
    unsigned short block;
    char data[DATA_SIZE];
}tftp_data;

typedef enum _tftp_error
{
    TFTP_ERR_UNDEFINED              = 0,
    TFTP_ERR_MODE_ERROR             = 1,
    TFTP_ERR_FILE_PATH_ERROR        = 2,
    TFTP_ERR_FILE_CREATE_FAIL       = 3,
    TFTP_ERR_FILE_NOT_FOUND         = 4,
    TFTP_ERR_FILE_CHECK_ERROR       = 5,
    TFTP_ERR_PROCESS_START_FAIL     = 6,
    TFTP_ERR_PROCESS_ALREADY_RUN    = 7,
    TFTP_ERR_PROCESS_STOP_FAIL      = 8,
    TFTP_ERR_ACCESS_DENIED          = 9,
    TFTP_ERR_DISK_FULL              = 10,
    TFTP_ERR_UNEXPECTED_OPCODE      = 11,
    TFTP_ERR_UNKNOWN_TRANSFER_ID    = 12,
    TFTP_ERR_FILE_ALREADY_EXISTS    = 13,
    TFTP_ERR_PROCESS_REBOOT_SYS     = 14,
    TFTP_ERR_PROCESS_REBOOT_APP     = 15,
    TFTP_ERR_PROCESS_UNCOMPRESS     = 16,
    TFTP_ERR_PROCESS_CHMOD          = 17,
    TFTP_ERR_PROCESS_UPGRADE        = 18,
}tftp_error_e;

//cmd
#define CMD_RRQ                 (short)1
#define CMD_WRQ                 (short)2
#define CMD_DATA                (short)3
#define CMD_ACK                 (short)4
#define CMD_ERROR               (short)5
#define CMD_LIST                (short)6
#define CMD_HEAD                (short)7
#define CMD_PROCESS_START       (short)8
#define CMD_PROCESS_STOP        (short)9
#define CMD_PROCESS_RESTART     (short)10
#define CMD_FILE_CHECK          (short)11
#define CMD_PROCESS_UPGRADE     (short)12
#define CMD_PROCESS_REBOOT_SYS  (short)13
#define CMD_PROCESS_REBOOT_APP  (short)14

struct tftpx_request{
    int size;
    struct sockaddr_in client;
    struct tftpx_packet packet;
};

extern int tftp_start_run(unsigned char *root_dir);


#endif


