//
//  F4PRouteUpgrade.m
//  F4-Plus
//
//  Created by lsq on 2017/7/18.
//  Copyright © 2017年 detu. All rights reserved.
//

#import "RouteUpgrade.h"
#import "tftp_client.h"

static const int f4p_tftp_server_port = 12;
static char *f4p_tftp_server_ip = "192.168.111.1";

@interface RouteUpgrade ()

@property (nonatomic, copy) progressBlock progress;
@property (nonatomic, copy) completeBlock complete;

@end

@implementation RouteUpgrade

static id object;

/*
建立socket连接
 */
- (void)f4pTftpClientInit{
    tftp_do_init(f4p_tftp_server_port, f4p_tftp_server_ip);
}

/*
 相机route升级
 */
- (void)updateRoute:(NSString *)fileName filePath:(NSString *)filePath progress:(progressBlock)progressBlock complete:(completeBlock)completeBlock{
    [self f4pTftpClientInit];
    self.progress = progressBlock;
    self.complete = completeBlock;
    object = self;
}
/*
 校验升级文件
 */
- (BOOL)checkFile:(NSString *)fileName{
    int result = tftp_do_check_file((char *)[fileName UTF8String]);
    if (result == 0) {
        NSLog(@"校验升级文件传输成功");
        return YES;
    }else{
        NSLog(@"校验升级文件传输失败");
        return NO;
    }
}

/*
 烧录
 */
- (void)updateStart:(resultBlock)result{
    int results = do_upgrade();
    if (result) {
        result(results);
    }
}

void f4p_progressBlock(int progress){
    [object getProgress:progress];
}

void f4p_completeBlock(char *error){
    [object getCompleteBlock:error];
}

// 进度回调
- (void)getProgress:(float)progress{
    if (self.progress) {
        self.progress(progress);
    }
}

// 错误回调
- (void)getCompleteBlock:(char *)error{
    NSString *errorStr = [NSString stringWithFormat:@"%s",error];
    if (self.complete) {
        self.complete(errorStr);
    }
}

@end
