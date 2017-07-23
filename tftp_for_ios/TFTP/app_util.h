#ifndef APP_UTIL_H
#define APP_UTIL_H

#if 0
#define SOFT_VERSION        "1.0"
#define DETU_APP_PATH       "/home/amba_work/f4_plus/f4_plus_loader/app"
#define DETU_APP_NAME       "f4_plus_app"
#define DETU_APP_ROOT_PATH  "/home/amba_work/f4_plus/f4_plus_loader" //end not has '/'
#else
#define SOFT_VERSION        	"1.1"
#define DETU_UPGRADE_FILENAME	"router_app.tar.gz"
#define DETU_UPGRADE_FILEPATH  	"/customer/detu/router_app.tar.gz"

#define DETU_APP_PATH       "/customer/detu/f4_plus_app"
#define DETU_APP_NAME       "f4_plus_app"
#define DETU_RTSP_SERVER    "f4_rtsp_server"
#define DETU_APP_ROOT_PATH  "/customer/detu" //end not has '/'

#endif


extern  int process_is_run(char *process_name);

extern int process_start_run(char *process_path);

extern int process_file_permission_update(char *process_path);

int process_stop_run(char *process_name);
int is_file_exist(char *path);
int process_reboot_sys();
int process_reboot_app();
int process_uncompress(char *filepath, char *dest);
int process_upgrade();

#endif

