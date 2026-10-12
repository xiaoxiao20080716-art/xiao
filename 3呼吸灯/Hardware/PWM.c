#include "stm32f10x.h"                  // 芯片的“说明书”头文件（Device header）
#include "PWM.h"                        // 本文件对应的头文件

/*==========================================================================
 * 函数名：PWM_Init
 * 作用  ：把 PA0 这根引脚配置成 PWM 输出，让它往外发方波
 *          本例子的 PWM 频率固定是 100Hz（周期10ms），占空比可以 0%~100% 随便调
 * 参数  ：无
 * 返回  ：无
 *========================================================================*/
void PWM_Init(void)
{
	/*--------------------------------------------------------------
	 * 第1步：打开“电源开关”
	 * STM32 里每个外设默认都是断电的（不用就不耗电），
	 * 想用哪个外设，就得先把它所在的“总线”通电。
	 *-------------------------------------------------------------*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    // 给定时器 TIM2 通电（TIM2 挂在 APB1 上）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 给 GPIOA 端口通电（PA0 属于 GPIOA）

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	
	
	/*--------------------------------------------------------------
	 * 第2步：配置 PA0 引脚
	 * PA0 正好是 TIM2“通道1”默认的输出脚，
	 * 所以要把它设置成“复用推挽输出”：这根引脚交给定时器去管，
	 * 定时器让它高就高、让它低就低，这样才能输出 PWM 波形。
	 *-------------------------------------------------------------*/
	GPIO_InitTypeDef GPIO_InitStructure;                     // 定义一个结构体，用来装引脚的各项参数
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          // 复用推挽输出（AF=交给片上外设控制，PP=推挽）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;                // 选第0号引脚，也就是 PA0
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;        // 引脚电平翻转速度，50MHz 足够快
	GPIO_Init(GPIOA, &GPIO_InitStructure);                   // 把上面填好的参数正式写进 GPIOA

	/*--------------------------------------------------------------
	 * 第3步：选时钟来源
	 * 定时器要数数，就得有个“节拍”。这里用芯片自带的 72MHz 内部时钟。
	 *-------------------------------------------------------------*/
	TIM_InternalClockConfig(TIM2);

	/*--------------------------------------------------------------
	 * 第4步：配置时基单元（决定“数数的步子多大”和“数满一轮要多少下”）
	 * 算一遍你就懂了：
	 *   72MHz（芯片主频） ÷ 7200 = 10000 次/秒   ← 每秒数1万下（这是“步子”）
	 *   数满一轮要 100 下：10000 ÷ 100 = 100 次/秒
	 * 所以 PWM 频率 = 100Hz，也就是每 10ms 走完一个完整的方波周期。
	 *-------------------------------------------------------------*/
	TIM_TimeBaseInitTypeDef TIM_TimBaseInitStructure;                 // 定义结构体，装时基参数
	TIM_TimBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;        // 时钟分频，跟PWM没关系，保持默认的1分频
	TIM_TimBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;    // 向上计数：0、1、2、3……一直往上加
	TIM_TimBaseInitStructure.TIM_Period = 100 - 1;                    // ARR自动重装值=99：从0数到99正好100下
	TIM_TimBaseInitStructure.TIM_Prescaler = 7200 - 1;                // PSC预分频=7199：把 72MHz 降成 10000Hz
	TIM_TimBaseInitStructure.TIM_RepetitionCounter = 0;               // 重复计数器，只有高级定时器才有，这里用不着，给0
	TIM_TimeBaseInit(TIM2, &TIM_TimBaseInitStructure);                // 把参数写进 TIM2

	/*--------------------------------------------------------------
	 * 第5步：配置输出比较单元（决定 PWM 波形长什么样）
	 * PWM 的核心原理：定时器一直在数数，
	 *     数到的值 < CCR   → 引脚输出高电平
	 *     数到的值 ≥ CCR   → 引脚输出低电平
	 * 所以 CCR 越大，高电平时间越长，占空比就越大，灯就越亮。
	 *-------------------------------------------------------------*/
	TIM_OCInitTypeDef TIM_OCInitStructure;                            // 定义结构体，装输出比较参数
	TIM_OCStructInit(&TIM_OCInitStructure);                           // 先给结构体填上默认值，防止没赋值的成员是乱数据
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;                 // 使用 PWM 模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;         // 高电平为“有效”，不反相（灯就是高电平亮）
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;     // 打开输出，引脚才会真的往外发波形
	TIM_OCInitStructure.TIM_Pulse = 0;                                // CCR初值=0，即一开始占空比 0%

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);                          // 配置 TIM2 的通道1（对应的就是 PA0 这根引脚）

	/*--------------------------------------------------------------
	 * 第6步：启动定时器，一启动 PWM 就开始往外输出了
	 *-------------------------------------------------------------*/
	TIM_Cmd(TIM2, ENABLE);
}

/*==========================================================================
 * 函数名：PWM_SetCompare1
 * 作用  ：修改 PWM 的占空比（调整“亮多久、灭多久”的比例）
 * 参数  ：Compare —— 比较值，范围 0~100
 *             0  → 占空比 0%  （一直低电平，相当于关掉）
 *            50  → 占空比 50% （一半时间亮、一半时间灭）
 *           100  → 占空比 100%（一直高电平，相当于全开）
 * 返回  ：无
 * 说明  ：本质就是往 CCR1 寄存器里写一个数。
 *          引脚高电平的时间 ÷ 一个周期的时间 = Compare ÷ 100
 *========================================================================*/
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);   // 把 Compare 写进 TIM2 的 CCR1 寄存器，占空比立刻改变
}
