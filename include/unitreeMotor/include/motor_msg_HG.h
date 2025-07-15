#ifndef __MOTOR_MSG_G1_H
#define __MOTOR_MSG_G1_H

#include <stdint.h>

#pragma pack(1)
/**
 * @brief 控制数据包格式
 * 
 */
typedef struct
{
    uint8_t head[2];    // 包头         2Byte
    uint8_t    id      : 4;   // 电机ID       0~14  15表示向所有电机广播数据(此时无返回)
    uint8_t    status  : 3;
    uint8_t    timeout : 1;
    uint8_t    res; 
    int16_t tor_des;        // 期望关节输出扭矩 unit: N.m     (q8)
    int16_t spd_des;        // 期望关节输出速度 unit: rad/s   (q7)
    int32_t pos_des;        // 期望关节输出位置 unit: rad     (q15)
    uint16_t  k_pos;        // 期望关节刚度系数 unit: 0.0-1.0 (q15)
    uint16_t  k_spd;        // 期望关节阻尼系数 unit: 0.0-1.0 (q15)
    uint32_t    CRC32;

} ControlData_hg;    // 主机控制命令     20Byte

/**
 * @brief 电机反馈数据包格式
 * 
 */
typedef struct
{
    uint8_t    head[2];       // 数据包头      0xFC 0xEE  不参与CRC

    uint8_t    id      : 4;   // 电机ID       0~14  15保留
    uint8_t    status  : 3;   // 工作模式      0.停机（上电默认值） 1.FOC  2~7.保留
    uint8_t    timeout : 1;   // 超时保护      0.没有超时  1.触发超时保护(需要控制位发0清除)

    int8_t     temp1;         // 壳体温度      -100~127(-128~-101保留)  1表示1℃
    uint8_t    temp2;         // 绕组温度      0~255    1表示1℃
    uint8_t    vol;           // 电机端电压    0~255    2表示1V
    int16_t    torque;        // 当前转子扭矩  -32768~32767             256表示：1Nm（其中4010电机为0.1Nm）
    int16_t    speed;         // 当前转子速度  -32768~32767             256/2π表示：1rad/s
    int32_t    pos;           // 当前转子位置  -2147483648~2147483647   32768/2π表示：1rad
    uint32_t   MError;        // 电机标识:     0.正常 按位表示，详见表1
    uint8_t    ExData[4];     // 保留
    uint32_t   CRC32;         // CRC32-MPEG2  注意不包含数据包头
    
} MotorData_hg;      // 电机返回数据     26Byte

#pragma pack()

#endif






