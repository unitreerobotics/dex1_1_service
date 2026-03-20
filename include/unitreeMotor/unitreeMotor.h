/**
 * @file unitreeMotor.h
 * @brief Unitree Motor SDK - Motor control and data structures
 * @author Unitree
 * @version 1.0
 * @date 2024
 *
 * This header defines the motor types, control modes, command structures,
 * and data structures for communicating with Unitree motors via serial interface.
 */

#ifndef __UNITREEMOTOR_H
#define __UNITREEMOTOR_H

#include "unitreeMotor/include/motor_msg_GO-M8010-6.h"
#include "unitreeMotor/include/motor_msg_A1B1.h"
#include "unitreeMotor/include/motor_msg_HG.h"
#include <stdint.h>
#include <iostream>

/**
 * @brief Enumeration of supported motor types with their default baud rates
 */
enum class MotorType{
    A1,         ///< 4.8M baudrate
    B1,         ///< 6.0M baudrate
    GO_M8010_6, ///< 4.0M baudrate
    M4010,      ///< 6.0M baudrate
    M5020,      ///< 6.0M baudrate
    M7520_14,   ///< 6.0M baudrate
    M7520_22    ///< 6.0M baudrate
};

/**
 * @brief Enumeration of motor control modes
 */
enum class MotorMode{
    BRAKE,      ///< Brake mode (motor locked)
    FOC,        ///< Field-oriented control mode
    CALIBRATE   ///< Calibration mode
};

/**
 * @brief Motor command structure for sending control commands to the motor
 */
struct MotorCmd{
    public:
        /**
         * @brief Default constructor
         */
        MotorCmd(){}

        MotorType motorType;        ///< Motor type
        int hex_len;                ///< Length of the hexadecimal data packet
        unsigned short id;          ///< Motor ID
        unsigned short mode;        ///< Control mode (see MotorMode)
        float tau;                  ///< Target torque (current) in N·m
        float dq;                   ///< Target velocity in rad/s
        float q;                    ///< Target position in rad
        float kp;                   ///< Position gain (proportional)
        float kd;                   ///< Velocity gain (derivative)
        unsigned short timeout;     ///< Timeout value for command validity

        /**
         * @brief Modify and prepare motor data for transmission
         * @param motor_s Pointer to the motor command structure
         */
        void modify_data(MotorCmd* motor_s);

        /**
         * @brief Get the prepared motor send data buffer
         * @return Pointer to the data buffer ready for transmission
         */
        uint8_t* get_motor_send_data();

        COMData32 Res;      ///< Reserved data field (32-bit)
        uint8_t res;        ///< Reserved byte
    private:
        ControlData_t  GO_M8010_6_motor_send_data;  ///< Internal data for GO_M8010_6 motor type
        ControlData_hg  HG_motor_send_data;         ///< Internal data for HG motor type
        MasterComdDataV3  A1B1_motor_send_data;     ///< Internal data for A1/B1 motor type

};

/**
 * @brief Motor data structure for receiving state feedback from the motor
 */
struct MotorData{
    public:
        /**
         * @brief Default constructor
         */
        MotorData(){}

        MotorType motorType;        ///< Motor type
        int hex_len;                ///< Length of the hexadecimal data packet
        unsigned char motor_id;     ///< Motor ID
        unsigned char mode;         ///< Current control mode
        int temp;                   ///< Motor temperature in Celsius
        int merror;                 ///< Error code/state
        float tau;                  ///< Measured torque (current) in N·m
        float dq;                   ///< Measured velocity in rad/s
        float q;                    ///< Measured position in rad

        bool correct = false;       ///< Flag indicating if received data is valid/correct

        /**
         * @brief Extract data from received motor response
         * @param motor_r Pointer to the motor data structure to fill
         * @return True if data extraction was successful
         */
        bool extract_data(MotorData* motor_r);

        /**
         * @brief Get the motor receive data buffer
         * @return Pointer to the data buffer for receiving
         */
        uint8_t* get_motor_recv_data();

        int footForce;              ///< Foot force sensor reading (if applicable)
        float LW;                   ///< Unknown (likely leg-related measurement)
        int Acc;                    ///< Accelerometer reading

        float gyro[3];              ///< Gyroscope readings [x, y, z] in rad/s
        float acc[3];               ///< Accelerometer readings [x, y, z] in m/s²

        unsigned short timeout;     ///< Timeout value for data validity
    private:
        MotorData_t GO_M8010_6_motor_recv_data;     ///< Internal data for GO_M8010_6 motor type
        MotorData_hg HG_motor_recv_data;            ///< Internal data for HG motor type
        ServoComdDataV3 A1B1_motor_recv_data;       ///< Internal data for A1/B1 motor type
};

// Utility Functions

/**
 * @brief Query the numeric value corresponding to a motor mode
 * @param motortype The motor type
 * @param motormode The motor mode
 * @return The integer value of the motor mode for the specified motor type
 */
int queryMotorMode(MotorType motortype, MotorMode motormode);

/**
 * @brief Query the gear ratio of a specific motor type
 * @param motortype The motor type
 * @return The gear ratio of the motor
 */
float queryGearRatio(MotorType motortype);

#endif  // UNITREEMOTOR_H