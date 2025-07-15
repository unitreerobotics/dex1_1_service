<div align="center">
  <h1 align="center">Dex1_1 service</h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
</div>

# 0. 📖 Introduction

This is a serial2dds service for the Dex1_1 gripper.

The Dex1_1 is a parallel two-finger gripper developed by Unitree. It uses a single unitree M4010 motor to drive the gripper.

<p align="center">
  <a href="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png">
    <img src="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png" alt="dex1-1 gripper" style="width: 45%;">
  </a>
</p>


# 1. 📦 Installation

```bash
# at user development computing unit PC2 (NVIDIA Jetson Orin NX board)
sudo apt install libspdlog-dev libboost-all-dev libyaml-cpp-dev libfmt-dev
cd ~
git clone https://github.com/unitreerobotics/dex1_1_service
cd dex1_1_service
mkdir build && cd build
cmake ..
make -j6
```

# 2. 🚀 Launch

```bash
# Run `sudo ./dex1_1_gripper_server -h` for details. The output will be:
# Unitree Dex1-1 Gripper Server:
#   -h [ --help ]                produce help message
#   -v [ --version ]             show version
#   -c [ --calibration ]         calibrate the gripper motor
#   -n [ --network ] arg (=eth0) dds networkInterface
#   -l [ --left ]                test left dex1 gripper
#   -r [ --right ]               test right dex1 gripper

# start server
sudo ./dex1_1_gripper_server --network eth0
# Simplified (defaults apply)
sudo ./dex1_1_gripper_server

# run test examples
sudo ./test_dex1_1_gripper_server --network eth0 -l -r
# Test only the left side or the right side individually.
sudo ./test_dex1_1_gripper_server --network eth0 -l
sudo ./test_dex1_1_gripper_server -r
```

# 3. 📏 Calibration

Close the gripper and run the following command to calibrate the gripper.

```bash
sudo ./dex1_1_gripper_server -c
```

------

Here is an example output from a calibration process.

```bash
unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_gripper_server -c
[2025-01-01 00:00:26.514] [info] Available Serial Ports: /dev/ttyUSB3, /dev/ttyUSB2, /dev/ttyUSB1, /dev/ttyUSB0
[2025-01-01 00:00:26.669] [info] Detected motors:
[2025-01-01 00:00:26.669] [info]   - Motor ID: 0         Side: Right     Port: /dev/ttyUSB2      cmdTopic: rt/dex1/Right/cmd     stateTopic: rt/dex1/Right/state
[2025-01-01 00:00:26.669] [info]   - Motor ID: 1         Side: Left      Port: /dev/ttyUSB1      cmdTopic: rt/dex1/Left/cmd      stateTopic: rt/dex1/Left/state
[2025-01-01 00:00:26.669] [info] ========== Motor Calibration (Motor 1 (index) of 2 (total)) ==========
[2025-01-01 00:00:26.669] [info]   - Motor ID: 0,        Side: Right,    Port: /dev/ttyUSB2
[2025-01-01 00:00:26.669] [info] Please manually close the gripper tightly. 
                                 Then press 's' + Enter to calibrate, or any other key to skip.
>
```

You need to manually close the gripper tightly, just like shown in the picture.

<p align="center">
  <a href="https://oss-global-cdn.unitree.com/static/34d3cbce3ab9404cb6c477a43004b269_1717x1407.png">
    <img src="https://oss-global-cdn.unitree.com/static/34d3cbce3ab9404cb6c477a43004b269_1717x1407.png" alt="close gripper" style="width: 45%;">
  </a>
</p>

After closing it, press the **s** key and then **Enter**.

```bash
> s
[2025-01-01 00:00:28.024] [info] Calibrating motor 0...
Motor type: MotorType::M4010
Id: 0
Calibration successful!
[2025-01-01 00:00:28.042] [info] Motor 0 calibration successful.
[2025-01-01 00:00:28.042] [info] ========== Motor Calibration (Motor 2 (index) of 2 (total)) ==========
[2025-01-01 00:00:28.042] [info]   - Motor ID: 1,        Side: Left,     Port: /dev/ttyUSB1
[2025-01-01 00:00:28.042] [info] Please manually close the gripper tightly. 
                                 Then press 's' + Enter to calibrate, or any other key to skip.
>
```

Same as the previous step, continue calibrating the second one.

```bash
> s
[2025-01-01 00:00:28.881] [info] Calibrating motor 1...
Motor type: MotorType::M4010
Id: 1
Calibration successful!
[2025-01-01 00:00:28.903] [info] Motor 1 calibration successful.
[2025-01-01 00:00:28.903] [info] Calibration process completed.
```


Check results.

```bash
unitree@ubuntu:~/dex1_1_service/build$ sudo ./test_dex1_1_gripper_server -l -r
# The gripper’s initial position should be near zero.
[2025-01-01 00:00:13.776] [info] Right gripper init at q = 0.001
[2025-01-01 00:00:14.978] [info] Left gripper init at q = 0.000
R= 0.508 L= 0.502
```