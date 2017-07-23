/*-------------------------------------------------------------------
 *function   :app_util
 *date       :2017-05-17
 *history    :
 *------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "app_util.h"

//check file exist
int is_file_exist(char *path)
{
    if (path == NULL)
        return -1;
    if (access(path, F_OK) == 0)
        return 0;
    
    return -1;
}

int process_uncompress(char *filepath, char *dest)
{
    FILE *ptr = NULL;
    char sh_cmd[256];
    int ret = -1;
    
    if(filepath==NULL){
        fprintf(stderr, "[Detu_Process]param error!\n");
        return -1;
    }
    
    sprintf(sh_cmd,"tar -zxvf %s -C %s", filepath, dest);
    if((ptr=popen(sh_cmd, "r")) != NULL){
        ret = 0;
    }else{
        fprintf(stderr, "[Detu_Process]permission_update fail!\n");
        ret = -1;
    }
    pclose(ptr);
    return ret;
}


//check a process if run or not
int process_is_run(char *process_name)
{
    FILE *ptr = NULL;
    char buff[512];
    char ps[64];
    int ret = -1;
    
    if(process_name==NULL){
        return -1;
    }
    
    sprintf(ps,"ps -e | grep -c \'%s\'",process_name);
    strcpy(buff,"ABNORMAL");
    
    if((ptr=popen(ps, "r")) != NULL)
    {
        ret = -1;
        while(fgets(buff, 512, ptr) != NULL)
        {
            
            if(atoi(buff) >= 3){
                ret = 0;
            }
        }
    }else{
        ret = -1;
    }
    pclose(ptr);
    return ret;
}


//start run a process by path
int process_start_run(char *process_path)
{
    FILE *ptr = NULL;
    char sh_cmd[256];
    int ret = 0;
    
    if(process_path==NULL){
        fprintf(stderr, "[Detu_Process]param error!\n");
        return -1;
    }
    
    sprintf(sh_cmd,"%s &",process_path);
#if 0
    if((ptr=popen(sh_cmd, "r")) != NULL){
        ret = 0;
    }else{
        fprintf(stderr, "[Detu_Process]start process fail!\n");
        ret = -1;
    }
    pclose(ptr);
#else
    system("/customer/detu/f4_plus_app &");
#endif
    return ret;
}

//stop process by name
int process_stop_run(char *process_name)
{
    FILE *ptr = NULL;
    char sh_cmd[256];
    int ret = -1;
    
    if(process_name==NULL){
        fprintf(stderr, "[Detu_Process]param error!\n");
        return -1;
    }
    
    sprintf(sh_cmd,"killall %s",process_name);
    if((ptr=popen(sh_cmd, "r")) != NULL){
        ret = 0;
    }else{
        fprintf(stderr, "[Detu_Process]stop process fail!\n");
        ret = -1;
    }
    pclose(ptr);
    return ret;
}

//stop process by name
int process_reboot_app()
{
    FILE *ptr = NULL;
    char sh_cmd[256];
    int ret = -1;
    int retry = 0;
    
    process_stop_run(DETU_APP_NAME);
    usleep(500*1000);
    //stop run Detu App
    for(retry=0; retry<3; retry++){
        if(0 == process_is_run(DETU_APP_NAME)){//still run
            process_stop_run(DETU_APP_NAME);
            usleep(500*1000);
        }else{
            break;
        }
    }
    if(retry >= 3){
        printf("[Detu_Loader]detu app has been run! \n");
        return -1;
    }
    //stop run rtsp server
    process_stop_run(DETU_RTSP_SERVER);
    usleep(500*1000);
    for(retry=0; retry<3; retry++){
        if(0 == process_is_run(DETU_RTSP_SERVER)){//still run
            process_stop_run(DETU_RTSP_SERVER);
            usleep(500*1000);
        }else{
            break;
        }
    }
    if(retry >= 3){
        printf("[Detu_Loader]detu app has been run! \n");
        return -1;
    }
    
    //find if the upgrade zip file exist,if 1, stop and remove all current and uncompress zip file
    //then restart
    if (0 == is_file_exist(DETU_UPGRADE_FILEPATH)) {
        ret = process_uncompress(DETU_UPGRADE_FILEPATH, DETU_APP_ROOT_PATH);
        if (ret != 0) {
            printf("[Detu_Loader]process_uncompress failed! \n");
            return -1;
        } else {
            //remove it
            unlink(DETU_UPGRADE_FILEPATH);
        }
        
        sprintf(sh_cmd, "%s/*", DETU_APP_ROOT_PATH);
        ret = process_file_permission_update(sh_cmd);
        if (ret != 0) {
            printf("[Detu_Loader]process_file_permission_update failed! \n");
            return -1;
        }
    }
    
    //start run detu f4_plus app
    ret = process_start_run(DETU_APP_PATH);
    if(ret != 0){
        printf("[Detu_Loader]start run app fail! \n");
        return -1;
    }
    
  	 return ret;
}



//stop process by name
int process_reboot_sys()
{
    FILE *ptr = NULL;
    char sh_cmd[256];
    int ret = -1;
    
    sprintf(sh_cmd,"reboot");
    system("reboot");
    return 0;
#if 0
    if((ptr=popen(sh_cmd, "r")) != NULL){
        ret = 0;
    }else{
        fprintf(stderr, "[Detu_Process]stop process fail!\n");
        ret = -1;
    }
    pclose(ptr);
    return ret;
#endif
    
}

int process_upgrade()
{
    char sh_cmd[256];
    //find if the upgrade zip file exist,if 1, stop and remove all current and uncompress zip file
    //then restart
    if (0 == is_file_exist(DETU_UPGRADE_FILEPATH)) {
        int ret = process_uncompress(DETU_UPGRADE_FILEPATH, DETU_APP_ROOT_PATH);
        if (ret != 0) {
            printf("[Detu_Loader]process_uncompress failed! \n");
            //return TFTP_ERR_PROCESS_UNCOMPRESS;
            return -1;
        } else {
            //remove it
            unlink(DETU_UPGRADE_FILEPATH);
        }
        
        sprintf(sh_cmd, "%s/*", DETU_APP_ROOT_PATH);
        ret = process_file_permission_update(sh_cmd);
        if (ret != 0) {
            printf("[Detu_Loader]process_file_permission_update failed! \n");
            //return TFTP_ERR_PROCESS_CHMOD;
            return -1;
        }
        
        return 0;
    }else {
        //return TFTP_ERR_FILE_NOT_FOUND;
        return -1;
    }
}
//
int process_file_permission_update(char *process_path)
{
    FILE *ptr = NULL;
    char sh_cmd[256]; 
    int ret = -1;
    
    if(process_path==NULL){
        fprintf(stderr, "[Detu_Process]param error!\n");
        return -1;
    }
    
    sprintf(sh_cmd,"chmod -R 777 %s",process_path);  
    if((ptr=popen(sh_cmd, "r")) != NULL){
        ret = 0; 
    }else{
        fprintf(stderr, "[Detu_Process]permission_update fail!\n");
        ret = -1;
    }
    pclose(ptr);
    return ret;  
}


