//
//  RouteUpgrade.h
//  F4-Plus
//
//  Created by lsq on 2017/7/18.
//  Copyright © 2017年 detu. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef void (^progressBlock)(int);
typedef void (^completeBlock)(NSString *);
typedef void (^resultBlock)(BOOL);

@interface RouteUpgrade : NSObject

/*
 相机route升级
 */
- (void)updateRoute:(NSString *)fileName filePath:(NSString *)filePath progress:(progressBlock)progressBlock complete:(completeBlock)completeBlock;

/*
 校验升级文件
 */
- (BOOL)checkFile:(NSString *)fileName;

/*
 烧录
 */
- (void)updateStart:(resultBlock)result;

@end
