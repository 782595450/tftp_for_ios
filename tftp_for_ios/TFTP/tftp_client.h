#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

// 建立socket连接
int tftp_do_init(int server_port,char *server_ips);

// 开始传输文件
void tftp_do_put(char *filename,char *filePath,void(*tftp_progress)(int progress),void(*tftp_complete)(char *error));

// 文件校验
int tftp_do_check_file(char *filename);

// 烧录
int do_upgrade();

// 重启系统
void do_reboot_sys();

// 重启应用
void do_reboot_app();

#endif

