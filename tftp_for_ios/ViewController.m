//
//  ViewController.m
//  tftp_for_ios
//
//  Created by lsq on 2017/7/23.
//  Copyright © 2017年 detu. All rights reserved.
//

#import "ViewController.h"
#import "RouteUpgrade.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    NSString *filepath = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"txt"];
    NSData *filedata = [NSData dataWithContentsOfFile:filepath];
    RouteUpgrade *route = [RouteUpgrade alloc];
    [route updateRoute:@"test.txt" filePath:filepath progress:^(int progress) {
        NSLog(@"update progress:%f",progress/(filedata.length*1.0));
    } complete:^(NSString *error) {
        NSLog(@"error : %@",error);
        if ([error isEqualToString:@"success"]) {
            [route checkFile:@"test.txt"];
            [route updateStart:^(BOOL result) {
                
            }];
        }else{
            NSLog(@"上传失败");
        }
    }];

}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
