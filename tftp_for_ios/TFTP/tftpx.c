/*-------------------------------------------------------------------
 *function   :tftp sever
 *date       :2017-05-17
 *history    :
 *------------------------------------------------------------------*/
#include "tftpx.h"
#include "app_util.h"

#define ROOT_DIR_PATH_MAX   64
char conf_document_root[ROOT_DIR_PATH_MAX] = {'\0'};

#define FILE_PATH_LEN       128
typedef struct _writed_file_info
{
    char file_name[FILE_PATH_LEN];
    char check[2];
}writed_file_info_t;

static writed_file_info_t writed_file_info_last;


// Send an ACK packet. Return bytes sent.
// If error occurs, return -1;
static int send_ack(int sock,unsigned short block)
{
    tftp_ack ack;
    ack.opcode = htons(CMD_ACK);
    ack.block = htons(block);
    if(send(sock, &ack, 4, 0) != sizeof(ack)){
        return -1;
    }
    return 0;
}

static int send_ack_error(int sock,unsigned short error)
{
    tftp_ack ack;
    ack.opcode = htons(CMD_ERROR);
    ack.block = htons(error);
    if(send(sock, &ack, 4, 0) != sizeof(ack)){
        return -1;
    }
    return 0;
}


// Send data and wait for an ACK. Return bytes sent.
// If error occurs, return -1;
static int send_packet(int sock, struct tftpx_packet *packet, int size)
{
    struct tftpx_packet rcv_packet;
    int time_wait_ack = 0;
    int rxmt = 0;
    int r_size = 0;
    
    for(rxmt = 0; rxmt < PKT_MAX_RXMT; rxmt ++){
        printf("Send block=%d\n", ntohs(packet->block));
        if(send(sock, packet, size, 0) != size){
            return -1;
        }
        for(time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 10000){
            // Try receive(Nonblock receive).
            r_size = recv(sock, &rcv_packet, sizeof(struct tftpx_packet), MSG_DONTWAIT);
            if(r_size >= 4 && rcv_packet.cmd == htons(CMD_ACK) && rcv_packet.block == packet->block){
                //printf("ACK: block=%d\n", ntohs(rcv_packet.block));
                // Valid ACK
                break;
            }
            usleep(10000);
        }
        if(time_wait_ack < PKT_RCV_TIMEOUT){
            break;
        }else{
            // Retransmission.
            continue;
        }
    }
    if(rxmt == PKT_MAX_RXMT){
        // send timeout
        printf("Sent packet exceeded PKT_MAX_RXMT.\n");
        return -1;
    }
    
    return size;
}

static void handle_list(int sock, struct tftpx_request *request)
{
    char fullpath[256];
    char data_buf[LIST_BUF_SIZE + 256 + 32];
    char *r_path = request->packet.data;	// request path
    int data_size;	// Size of data in data_buf[]
    struct tftpx_packet packet;
    
    if(strlen(r_path) + strlen(conf_document_root) > sizeof(fullpath) - 1){
        printf("Request path too long! ");
        return;
    }
    
    // build fullpath
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    printf("List %s\n", fullpath);
    
    struct stat stat_buf;
    struct dirent *dirent;
    DIR *dp;
    
    stat(fullpath, &stat_buf);
    if(!S_ISDIR(stat_buf.st_mode)){
        printf("\"%s\" is not a directory.\n", fullpath);
        packet.cmd = htons(CMD_ERROR);
        packet.code = 0;
        sprintf(packet.data, "\"%s\" is not a directory!\n", r_path);
        send_packet(sock, &packet, strlen(packet.data) + 4);
        return;
    }
    
    char *ptr = fullpath + strlen(fullpath);
    data_size = 0;
    dp = opendir(fullpath);
    
    // fill dir list buffer
    while((dirent = readdir(dp)) != NULL){
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0){
            continue;
        }
        ptr[0] = '/';
        strcpy(ptr + 1, dirent->d_name);
        *(ptr + 1 + strlen(dirent->d_name)) = '\0';
        stat(fullpath, &stat_buf);
        
        char mod = S_ISDIR(stat_buf.st_mode)? 'd' : 'f';	//
        data_size += sprintf(data_buf + data_size,
                             "%c\t%d\t%s\t\n",
                             mod, (int)stat_buf.st_size, dirent->d_name);
        if(data_size >= LIST_BUF_SIZE){
            // Too much files in a directory!
            printf("SEND_BUF_SIZE too small!\n");
            return;
        }
    }
    
    ushort block;
    packet.cmd = htons(CMD_DATA);
    for(block=1; block<data_size/DATA_SIZE + 1; block++){
        // Network byte order.
        packet.block = htons(block);
        memcpy(packet.data, data_buf + DATA_SIZE * (block - 1), DATA_SIZE);
        if(send_packet(sock, &packet, DATA_SIZE + 4) == -1){
            printf("Error occurs when sending Packet %d.", block);
            return;
        }
        usleep(PKT_TIME_INTERVAL);
    }
    // Now, send the last packet.
    packet.block = htons(block);
    memcpy(packet.data, data_buf + DATA_SIZE * (block - 1), data_size - DATA_SIZE * (block - 1));
    if(send_packet(sock, &packet, data_size - DATA_SIZE * (block - 1) + 4) == -1){
        printf("Error occurs when sending the last packet.");
        return;
    }
    
    printf("Sent %d packet(s).\n", block);
}


static void handle_rrq(int sock, struct tftpx_request *request)
{
    struct tftpx_packet snd_packet;
    char fullpath[256];
    char *r_path = request->packet.filename;	// request file
    char *mode = r_path + strlen(r_path) + 1;
    char *blocksize_str = mode + strlen(mode) + 1;
    int blocksize = atoi(blocksize_str);
    
    if(blocksize <= 0 || blocksize > DATA_SIZE){
        blocksize = DATA_SIZE;
    }
    
    if(strlen(r_path) + strlen(conf_document_root) > sizeof(fullpath) - 1){
        printf("[tftpX]Request path too long! \n");
        return;
    }
    
    // build fullpath
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]rrq: \"%s\", blocksize=%d\n", fullpath, blocksize);
    
    FILE *fp = fopen(fullpath, "r");
    if(fp == NULL){
        printf("[tftpX]File not exists!\n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }
    
    int s_size = 0;
    ushort block = 1;
    snd_packet.cmd = htons(CMD_DATA);
    do{
        memset(snd_packet.data, 0, sizeof(snd_packet.data));
        snd_packet.block = htons(block);
        s_size = fread(snd_packet.data, 1, blocksize, fp);
        if(send_packet(sock, &snd_packet, s_size + 4) == -1){
            fprintf(stderr, "[tftpX]Error occurs when sending packet.block = %d.\n", block);
            goto rrq_error;
        }
        block ++;
    }while(s_size == blocksize);
    
    printf("[tftpX]Send file end.\n");
    
rrq_error:
    fclose(fp);
    
    return;
}

static void handle_wrq(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    tftp_data data_packet;
    char fullpath[128];
    char fullpath_bak[128];
    char fullpath_old[128];
    char *r_path = request->packet.filename; // request file
    char *mode = r_path + strlen(r_path) + 1;
    char *blocksize_str = mode + strlen(mode) + 1;
    int blocksize = atoi(blocksize_str);
    int s_size = 0;
    int r_size = 0;
    int time_wait_data;
    unsigned short block = 1;
    FILE *fp_bak_file;
    FILE *fp_old_file;
    unsigned char check_1 = 0;
    unsigned char check_2 = 0;
    unsigned int i;
    
    if(blocksize <= 0 || blocksize > DATA_SIZE){
        blocksize = DATA_SIZE;
    }
    
    if(strlen(r_path) + strlen(conf_document_root) > sizeof(fullpath) - 1){
        printf("[tftpX]Request path too long! \n");
        send_ack_error(sock, TFTP_ERR_FILE_PATH_ERROR);
        return;
    }
    
    //build fullpath
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]write file:\"%s\", blocksize=%d, mode=%s \n", fullpath, blocksize, mode);
    
    //if bak file has been exist,need delete bak file first
    strcat(fullpath_bak,fullpath);
    //strcat(fullpath_bak,"_bak");
    fp_bak_file= fopen(fullpath_bak, "r");
    if(fp_bak_file != NULL){
        printf("[tftpX]bak file exist,now delete it \n");
        fclose(fp_bak_file);
        remove(fullpath_bak);
    }
    
    //create a bak file to save data
    fp_bak_file = fopen(fullpath_bak, "w");
    if(fp_bak_file == NULL){
        printf("[tftpX]create file fail! \n");
        send_ack_error(sock, TFTP_ERR_FILE_CREATE_FAIL);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        fprintf(stderr, "[tftpX]Error occurs when sending ACK = %d \n", 0);
        fclose(fp_bak_file);
        remove(fullpath_bak);
        goto wrq_error;
    }
    
    check_1 = 0;
    check_2 = 0;
    //start recv file data
    do{
        for(time_wait_data = 0; time_wait_data < PKT_RCV_RERTY_COUNT; time_wait_data++){
            
            // Try receive(Nonblock receive).
            r_size = recv(sock, &data_packet, sizeof(data_packet), MSG_DONTWAIT);
            if(r_size > 0 && r_size < 4){
                printf("[tftpX]Bad packet: r_size=%d, blocksize=%d\n", r_size, blocksize);
            }
            
            //check data
            if((r_size >= 4) && (data_packet.opcode == htons(CMD_DATA)) && (data_packet.block == htons(block))){
                //printf("[tftpX]DATA: block=%d, data_size=%d\n", ntohs(data_packet.block), r_size - 4);
                
                fwrite(data_packet.data, 1, r_size - 4, fp_bak_file);
                
                //cal check data
                for(i=0;i<r_size-4;i++){
                    check_1 = check_1 + data_packet.data[i];
                    check_2 = check_2 ^ data_packet.data[i];
                }
                
                break;
            }
            usleep(PKT_RCV_RERTY_INTERVAL);//time=20ms
        }
        if(time_wait_data >= PKT_RCV_RERTY_COUNT){
            printf("[tftpX]Receive timeout.\n");
            fclose(fp_bak_file);
            remove(fullpath_bak);
            goto wrq_error;
            break;
        }
        
        if(send_ack(sock, block) == -1){//send ack to client I has been recv data block
            fprintf(stderr, "[tftpX]Error occurs when sending ACK = %d.\n", block);
            fclose(fp_bak_file);
            remove(fullpath_bak);
            goto wrq_error;
            break;
        }
        
        block ++;
    }while(r_size == blocksize + 4);
    
    printf("[tftpX]Receive file end.\n");
    fclose(fp_bak_file);
    
    //save cur writed file'check data
    writed_file_info_last.check[0] = check_1;
    writed_file_info_last.check[1] = check_2;
    memset(writed_file_info_last.file_name, 0, sizeof(writed_file_info_last.file_name));
    strcat(writed_file_info_last.file_name, r_path);
    printf("[tftpX]file check,%s, %d, %d \n",writed_file_info_last.file_name, writed_file_info_last.check[0], writed_file_info_last.check[1]);
    
    //rename bak file
    strcat(fullpath_old,fullpath);
    strcat(fullpath_old,"_old");
    fp_old_file= fopen(fullpath_old, "r");
    if(fp_old_file != NULL){
        printf("[tftpX]bak file exist,now delete it \n");
        fclose(fp_old_file);
        remove(fullpath_old);
    }
    //rename(fullpath,fullpath_old);
    //remove(fullpath);
    //rename(fullpath_bak,fullpath);
    process_file_permission_update(fullpath);
    
wrq_error:
    
    return;
}

//optcode(2byte) + filename(\0,node:not path) + mode(octet/netascii \0)
static void handle_process_start(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char fullpath[256];
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    FILE *fp;
    unsigned int try = 0;
    char *process_name;
    
    //check file path
    if(strlen(r_path) + strlen(conf_document_root) > sizeof(fullpath) - 1){
        printf("[tftpX]process name too long! \n");
        send_ack_error(sock, TFTP_ERR_FILE_PATH_ERROR);
        return;
    }
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    //build fullpath
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    printf("[tftpX]start process name:\"%s\" \n", fullpath);
    
    //check if process has been run
    process_name = r_path;
    if(0 == process_is_run(process_name)){
        printf("[tftpX]process has been run! ");
        send_ack_error(sock, TFTP_ERR_PROCESS_ALREADY_RUN);
        return;
    }
    
    //check if file exist
    fp = fopen(fullpath, "r");
    if(fp == NULL){
        printf("[tftpX]not find process file! \n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }
    fclose(fp);
    
    //start run app
    process_start_run(fullpath);
    for(try=0;try<3;try++){
        if(0 == process_is_run(process_name)){//still run
            break;
        }else{
            process_start_run(fullpath);
        }
    }
    if(try >= 3){
        printf("[tftpX]process start fail! \n");
        send_ack_error(sock, TFTP_ERR_PROCESS_START_FAIL);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}


//optcode(2byte) + filename(\0,node:not path) + mode(\0)
static void handle_process_stop(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    unsigned int try = 0;
    char *process_name;
    
    //check file name
    if(strlen(r_path) > 64){
        printf("[tftpX]process name too long! \n");
        send_ack_error(sock, TFTP_ERR_FILE_PATH_ERROR);
        return;
    }
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]stop process name:\"%s\" \n",r_path);
    
    //check if process has been run
    process_name = r_path;
    process_stop_run(process_name);
    for(try=0;try<3;try++){
        if(0 == process_is_run(process_name)){//still run
            process_stop_run(process_name);
        }else{
            break;
        }
    }
    if(try >= 3){
        printf("[tftpX]stop process fail");
        send_ack_error(sock, TFTP_ERR_PROCESS_STOP_FAIL);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}

//optcode(2byte) + filename(\0,node:not path) + mode(\0)
static void handle_process_restart(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char fullpath[256];
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    FILE *fp;
    unsigned int try = 0;
    char *process_name;
    
    //check file name
    if(strlen(r_path) > 64){
        printf("[tftpX]process name too long! \n");
        send_ack_error(sock, TFTP_ERR_FILE_PATH_ERROR);
        return;
    }
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]restart process name:\"%s\" \n",r_path);
    
    //build full path
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    //check file is exsit
    fp = fopen(fullpath, "r");
    if(fp == NULL){
        printf("[tftpX]not find process file! \n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }
    fclose(fp);
    
    //check if process has been run
    process_name = r_path;
    process_stop_run(process_name);
    for(try=0;try<3;try++){
        if(0 == process_is_run(process_name)){//still run
            process_stop_run(process_name);
        }else{
            break;
        }
    }
    if(try >= 3){
        printf("[tftpX]stop process fail");
        send_ack_error(sock, TFTP_ERR_PROCESS_STOP_FAIL);
        return;
    }
    
    //start run app
    try = 0;
    process_start_run(fullpath);
    for(try=0;try<3;try++){
        if(0 == process_is_run(process_name)){//still run
            break;
        }else{
            process_start_run(fullpath);
        }
    }
    if(try >= 3){
        printf("[tftpX]process start fail! \n");
        send_ack_error(sock, TFTP_ERR_PROCESS_START_FAIL);
        return;
    }
    
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}

//optcode(2byte) + filename(\0,node:not path) + mode(\0)
static void handle_process_restart_app(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    unsigned int try = 0;
    char *process_name;
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]handle_process_restart_app \n");
    
    if(process_reboot_app() != 0){
        printf("[tftpX]process_reboot_app fail");
        send_ack_error(sock, TFTP_ERR_PROCESS_REBOOT_APP);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}

static void handle_process_restart_sys(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    unsigned int try = 0;
    char *process_name;
    
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]handle_process_restart_sys \n");
    
    if (!process_reboot_sys()) {
        printf("[tftpX] process_reboot_sys fail");
        send_ack_error(sock, TFTP_ERR_PROCESS_REBOOT_SYS);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}

static void handle_process_upgrade(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char *r_path = request->packet.filename;
    char *mode = r_path + strlen(r_path) + 1;
    unsigned int try = 0;
    char *process_name;
    
    //check mode
    if(!strncasecmp(mode, "octet", 5) && !strncasecmp(mode, "netascii", 8)){
        printf("[tftpX]mode error:%s ",mode);
        send_ack_error(sock, TFTP_ERR_MODE_ERROR);
        return;
    }
    
    printf("[tftpX]process_upgrade \n");
    
    if(process_upgrade() != 0){
        printf("[tftpX]process_upgrade fail");
        send_ack_error(sock, TFTP_ERR_PROCESS_UPGRADE);
        return;
    }
    
    //send ask to client can start send file data
    if(send_ack(sock, 0) == -1){
        printf("[tftpX]send ack fail! \n");
    }
    
    return;
}


//optcode(2byte) + filename(\0,node:not path) + check(2byte)
static void handle_file_check(int sock, struct tftpx_request *request)
{
    struct tftpx_packet ack_packet;
    char fullpath[128];
    char *r_path = request->packet.filename;
    char *check = r_path + strlen(r_path) + 1;
    FILE *fp;
    
    printf("[tftpX]file check,name:%s, check:%d, %d \n",r_path, *(check+0), *(check+1));
    
    //check file path
    if(strlen(r_path) + strlen(conf_document_root) > sizeof(fullpath) - 1){
        printf("[tftpX]file check,file name too long! \n");
        send_ack_error(sock, TFTP_ERR_FILE_PATH_ERROR);
        return;
    }
    
    //build fullpath
    memset(fullpath, 0, sizeof(fullpath));
    strcpy(fullpath, conf_document_root);
    if(r_path[0] != '/'){
        strcat(fullpath, "/");
    }
    strcat(fullpath, r_path);
    
    //check file is exsit
    fp = fopen(fullpath, "r");
    if(fp == NULL){
        printf("[tftpX]file check,not find file! \n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }
    fclose(fp);
    
    //
    if(strlen(writed_file_info_last.file_name) == 0){
        printf("[tftpX]file check,not find file from buff! \n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }
    
    if(strcmp(writed_file_info_last.file_name, r_path) != 0){//not find file name
        printf("[tftpX]file check,last file name error! \n");
        send_ack_error(sock, TFTP_ERR_FILE_NOT_FOUND);
        return;
    }else{
        if((writed_file_info_last.check[0] != *(check+0)) || (writed_file_info_last.check[1] != *(check+1)) ){
            printf("[tftpX]file check error! \n");
            remove(fullpath);
            send_ack_error(sock, TFTP_ERR_FILE_CHECK_ERROR);
            return;
        }
    }
    
    //send ask to client can start send file data
    send_ack(sock, 0);
    
    return;
}

static void *tftp_handler_thread(void *arg)
{
    int sock;
    struct tftpx_request *request = NULL;
    struct sockaddr_in server;
    static socklen_t addr_len = sizeof(struct sockaddr_in);
    
    if(NULL==arg){
        printf("[tftpX]handler_thread,arg error!\n");
    }
    
    request = (struct tftpx_request *)arg;
    if(request->size <= 0){
        printf("[tftpX]Bad request!\n");
        return NULL;
    }
    
    printf("[tftpX]tftp handler thread start run. \n");
    
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printf("[tftpX]handler_thread,socket could not be created!\n");
        return NULL;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0;
    
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("[tftpX]handler_thread,bind sock failed! \n");
        return NULL;
    }
    
    if(connect(sock, (struct sockaddr*)&(request->client), addr_len) < 0){
        printf("[tftpX]handler_thread,can't connect to client!\n");
        return NULL;
    }
    
    // Choose handler
    switch(request->packet.cmd){
        case CMD_RRQ:
            printf("[tftpX]handle_rrq called.\n");
            handle_rrq(sock, request);
            break;
        case CMD_WRQ:
            printf("[tftpX]handle_wrq called.\n");
            handle_wrq(sock, request);
            break;
        case CMD_LIST:
            printf("[tftpX]handle_list called.\n");
            handle_list(sock, request);
            break;
        case CMD_PROCESS_START:
            printf("[tftpX]handle_process_start called.\n");
            handle_process_start(sock, request);
            break;
        case CMD_PROCESS_STOP:
            printf("[tftpX]handle_process_stop called.\n");
            handle_process_stop(sock, request);
            break;
        case CMD_PROCESS_RESTART:
            printf("[tftpX]handle_process_restart called.\n");
            handle_process_restart(sock, request);
            break;
        case CMD_FILE_CHECK:
            printf("[tftpX]handle_file_check called.\n");
            handle_file_check(sock, request);
            break;
        case CMD_PROCESS_REBOOT_SYS:
            printf("[tftpX]handle_reboot system.\n");
            handle_process_restart_sys(sock, request);
            break;
        case CMD_PROCESS_REBOOT_APP:
            printf("[tftpX]handle_reboot app.\n");
            handle_process_restart_app(sock, request);
            break;
        case CMD_PROCESS_UPGRADE:
            printf("[tftpX]handle_process_upgrade .\n");
            handle_process_upgrade(sock, request);
            break;
        default:
            printf("[tftpX]Illegal tftp operation!\n");
            break;
    }
    
    //release msg
    free(request);
    close(sock);
    
    return NULL;
}

static void tftp_config_root_dir(unsigned char *root_dir)
{
    int i; 
    
    if(NULL==root_dir){
        return;
    }
    
    for(i=0;i<ROOT_DIR_PATH_MAX;i++){
        conf_document_root[i] = '\0';   
    } 
    
    strcpy(conf_document_root, root_dir);
    
}

int tftp_start_run(unsigned char *root_dir)
{
    int sock;
    int done = 0;
    socklen_t addr_len;
    pthread_t t_id;
    struct sockaddr_in server;
    unsigned short port = SERVER_PORT;
    struct tftpx_request *request;
    int i;
    
    printf("[tftpX]tftp_start_run \n");
    
    //config root dir
    tftp_config_root_dir(root_dir);
    
    //
    for(i=0;i<FILE_PATH_LEN;i++){
        writed_file_info_last.file_name[i] = '\0';
    }
    writed_file_info_last.check[0] = 0;
    writed_file_info_last.check[1] = 0;
    
    //config server socket
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printf("[tftpX]Server socket could not be created!\n");
        return 0;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if(bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
        printf("[tftpX]Server bind failed. Server already running? Proper permissions?\n");
        return 2;
    }	
    printf("[tftpX]Server started at port:%d\n",port);
    
    //start listen server socket,wait client connect
    addr_len = sizeof(struct sockaddr_in);
    while(!done){
        request = (struct tftpx_request *)malloc(sizeof(struct tftpx_request));
        memset(request, 0, sizeof(struct tftpx_request));
        request->size = recvfrom(
                                 sock, &(request->packet), MAX_REQUEST_SIZE, 0,
                                 (struct sockaddr *) &(request->client),
                                 &addr_len); 
        request->packet.cmd = ntohs(request->packet.cmd);
        printf("[tftpX]Receive request.\n"); 
        if(pthread_create(&t_id, NULL, tftp_handler_thread, request) != 0){
            printf("[tftpX]Can't create thread!\n");
        }
    }
    
    return 0;
}


