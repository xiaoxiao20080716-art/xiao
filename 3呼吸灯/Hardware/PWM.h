#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"                     // 引入芯片头文件，里面才有 uint16_t 这种类型

void PWM_Init(void);                       // 初始化PWM：把PA0引脚配置成PWM输出
void PWM_SetCompare1(uint16_t Compare);    // 设置占空比：Compare 取值范围 0~100，代表 0%~100%



#endif
