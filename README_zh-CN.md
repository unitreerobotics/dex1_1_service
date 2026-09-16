<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com/cn/Dex1-1" target="_blank">Dex1_1 Service（内走线）</a>
  </h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
  <p align="center">
    <a href="README.md"> English </a> | <a> 中文 </a> </a>
  </p>
</div>

# 0. 📖 介绍

> ⚠️ 这是 **`internal` 分支**，面向**内走线**的 Dex1-1 夹爪。如果你的夹爪是外走线的（串口板插在机器人的 USB 口上），请使用 `main` 分支。

Dex1_1 是 Unitree 开发的夹爪，为具⾝智能应⽤⽽⽣，由一个 M4010 电机驱动。

内走线时，夹爪电机直接接入机器人本体的电机总线，因此**不需要运行 serial2dds 服务**。运行期夹爪由机器人控制器作为低层指令的一部分驱动，对应 `rt/lowcmd` 的 `31`（左）和 `33`（右）号电机，与手臂走同一条通路。

本仓库在这个分支上只保留标定功能。内走线夹爪电机的 ID 是 **7**，而不是外走线的 ID 0 / ID 1，所以本分支的标定程序只与 ID 7 通信，不做其他任何事情。

<p align="center">
  <a href="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png">
    <img src="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png" alt="dex1-1 gripper" style="width: 45%;">
  </a>
</p>

# 1. 📦 安装

```bash
# 在用户开发计算单元 PC2（NVIDIA Jetson Orin NX 板）
sudo apt update
sudo apt install libserialport-dev
sudo apt install libspdlog-dev libfmt-dev libboost-program-options-dev
cd ~
git clone https://github.com/unitreerobotics/dex1_1_service
cd dex1_1_service
git checkout internal
mkdir build && cd build
cmake ..
make -j6
```

> 💡 注意：在 `lib` 目录下提供了上述安装命令中 sudo apt install libserialport-dev 的离线 deb 文件（Ubuntu 22.04），同时提供 `arm64`（Jetson）和 `amd64`（x86_64）两种架构，以供用户方便离线安装。如果你在安装 libserialport 时遇到问题，可以尝试安装这两个 deb 文件。要注意安装顺序，以下是安装命令示例：
> ```bash
> # arm64（NVIDIA Jetson Orin NX）
> sudo dpkg -i lib/libserialport0_0.1.1-3_arm64.deb
> sudo dpkg -i lib/libserialport-dev_0.1.1-3_arm64.deb
>
> # amd64 / x86_64 PC
> sudo dpkg -i lib/libserialport0_0.1.1-3_amd64.deb
> sudo dpkg -i lib/libserialport-dev_0.1.1-3_amd64.deb
> ```

# 2. 📏 标定

一次标定一个夹爪。程序除了标定不做别的事：扫描串口、寻找 ID 7 电机、提示你闭合夹爪，然后退出。

> ⚠️ 左右两个内走线夹爪的电机 ID 都是 **7**，无法区分，因此**每次只能接入一个 Dex1-1 夹爪**。如果程序在多个串口上都发现了 ID 7，它会拒绝标定并以状态 `1` 退出。

```bash
sudo ./dex1_1_internal_calibration
```

---

以下是标定过程示例输出。

```bash
unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
[2025-01-01 00:00:26.514] [info] Available Serial Ports: /dev/ttyUSB1, /dev/ttyUSB0
[2025-01-01 00:00:26.669] [info] ========== Motor Calibration ==========
[2025-01-01 00:00:26.669] [info]   - Motor ID: 7,        Port: /dev/ttyUSB0
[2025-01-01 00:00:26.669] [info] Please manually close the gripper tightly.
                                 Then press 's' + Enter to calibrate, or any other key to skip.
>
```

你需要像图中一样手动紧闭夹爪。

<p align="center">
  <a href="https://oss-global-cdn.unitree.com/static/34d3cbce3ab9404cb6c477a43004b269_1717x1407.png">
    <img src="https://oss-global-cdn.unitree.com/static/34d3cbce3ab9404cb6c477a43004b269_1717x1407.png" alt="close gripper" style="width: 45%;">
  </a>
</p>

紧闭合后，按 **s** 键，然后 **Enter**。

```bash
> s
[2025-01-01 00:00:28.024] [info] Calibrating motor 7...
Motor type: MotorType::M4010
Id: 7
Calibration successful!
[2025-01-01 00:00:28.042] [info] Motor 7 calibration successful.
[2025-01-01 00:00:28.042] [info] Calibration process completed.
```

标定成功时程序返回 `0`；未找到电机或标定失败时返回 `1`。

# 3. ✅ 验证

`test/test_internal_dex1.py` 通过 `rt/lowcmd` 控制两个夹爪张开和闭合，并打印实测位置。双臂**不会**运动：每个周期每个关节的指令都取自它自己的实测位置，所以机器人保持当前姿态，只有 31 / 33 号电机被驱动。

```bash
python3 test/test_internal_dex1.py --network-interface eth0
```

```bash
[G1_29_Arm_Internal_Dex1_Controller] Subscribe dds ok.
Current dual gripper q: [0.001 0.000]
G1_29 internal Dex1 verifier:
-----------------------------------
  open: left=1.832 right=1.845
  open: left=3.671 right=3.680
  ...
  close: left=0.412 right=0.405
```

夹爪位置范围大致是 `0.0`（闭合）到 `5.4`（张开）。如果数值始终不动，说明上一步的标定没有生效，或者机器人控制器没有接受 `rt/lowcmd`。

> ⚠️ 该脚本发布的是 `rt/lowcmd`，即全身低层指令话题。运行前请确认没有其他程序同时在控制机器人，并让机器人处于安全状态。

> 💡 需要 Python 环境中已安装 `unitree_sdk2py` 和 `logging_mp`。

# ❓ 常见问题

1. 标定程序本身不需要 DDS：它不链接 unitree_sdk2 和 CycloneDDS，只用到 libserialport、spdlog/fmt 和 Boost.program_options。如果 `cmake ..` 报缺包，检查上面几条 `apt install` 是否都执行成功。

   第 3 节的 Python 验证脚本是另一回事，它确实需要 `unitree_sdk2py`：

   ```bash
   cd ~
   git clone https://github.com/unitreerobotics/unitree_sdk2_python
   cd unitree_sdk2_python
   pip3 install -e .
   ```

2. 找不到电机：

   ```bash
   unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
   [2025-08-14 09:56:53.595] [info] Available Serial Ports: /dev/ttyUSB1, /dev/ttyUSB0
   [2025-08-14 09:56:54.339] [error] Dex1-1 gripper motor (ID 7) not found.
   # 或
   unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
   [2025-08-14 09:58:12.010] [info] Available Serial Ports:
   [2025-08-14 09:58:12.010] [error] No supported serial ports found.
   ```

   可能的原因：

   1. 夹爪电源未连接，或线路接触不良。
   2. 夹爪与机器人电机总线之间的内部线缆未接好。
   3. 你的夹爪是**外走线**的，因此电机 ID 是 0 / 1 而不是 7 —— 请改用 `main` 分支。
