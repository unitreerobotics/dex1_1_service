<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com/Dex1-1" target="_blank">Dex1_1 Service (internal routing)</a>
  </h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
  <p align="center">
    <a> English </a> | <a href="README_zh-CN.md">中文</a> </a>
  </p>
</div>

# 0. 📖 Introduction

> ⚠️ This is the **`internal` branch**, for the **internally-routed** Dex1-1 gripper. If your gripper is externally routed (its serial board plugs into a USB port on the robot), use the `main` branch instead.

The Dex1_1 is a parallel two-finger gripper developed by Unitree. It uses a single unitree M4010 motor to drive the gripper.

With internal routing the gripper motor is wired into the robot's own motor bus, so there is **no serial2dds service to run**. At runtime the gripper is driven by the robot controller as part of the low-level command, through motors `31` (left) and `33` (right) of `rt/lowcmd` — the same path the arms use.

What is left for this repository is calibration. The internally-routed gripper motor answers on **ID 7**, not on ID 0 / ID 1 like the externally-routed one, so the calibration program on this branch talks to ID 7 only and does nothing else.

<p align="center">
  <a href="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png">
    <img src="https://oss-global-cdn.unitree.com/static/3077509a2c6e4a9ebe1e57d45a42d1af_1796x1420.png" alt="dex1-1 gripper" style="width: 45%;">
  </a>
</p>

# 1. 📦 Installation

```bash
# at user development computing unit PC2 (NVIDIA Jetson Orin NX board)
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

> 💡 Note: The `lib` directory provides offline packages for `sudo apt install libserialport-dev` (Ubuntu 22.04), for both `arm64` (Jetson) and `amd64` (x86_64) machines. If you have trouble installing `libserialport`, you can try installing these two deb packages. Install `libserialport0` first, then `libserialport-dev`:
> ```bash
> # arm64 (NVIDIA Jetson Orin NX)
> sudo dpkg -i lib/libserialport0_0.1.1-3_arm64.deb
> sudo dpkg -i lib/libserialport-dev_0.1.1-3_arm64.deb
>
> # amd64 / x86_64 PC
> sudo dpkg -i lib/libserialport0_0.1.1-3_amd64.deb
> sudo dpkg -i lib/libserialport-dev_0.1.1-3_amd64.deb
> ```

# 2. 📏 Calibration

Calibrate one gripper at a time. Running the program does nothing but calibrate: it scans the serial ports, looks for motor ID 7, asks you to close the gripper, and exits.

> ⚠️ Both the left and the right internally-routed gripper answer on motor **ID 7**, so they cannot be told apart. **Connect only one Dex1-1 gripper at a time.** If the program sees ID 7 on more than one port it refuses to calibrate and exits with status `1`.

```bash
sudo ./dex1_1_internal_calibration
```

------

Here is an example output from a calibration process.

```bash
unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
[2025-01-01 00:00:26.514] [info] Available Serial Ports: /dev/ttyUSB1, /dev/ttyUSB0
[2025-01-01 00:00:26.669] [info] ========== Motor Calibration ==========
[2025-01-01 00:00:26.669] [info]   - Motor ID: 7,        Port: /dev/ttyUSB0
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
[2025-01-01 00:00:28.024] [info] Calibrating motor 7...
Motor type: MotorType::M4010
Id: 7
Calibration successful!
[2025-01-01 00:00:28.042] [info] Motor 7 calibration successful.
[2025-01-01 00:00:28.042] [info] Calibration process completed.
```

The program exits with status `0` on success and `1` if the motor was not found or calibration failed.

# 3. ✅ Verification

`test/test_internal_dex1.py` opens and closes both grippers over `rt/lowcmd` and prints their measured positions. The arms are **not** moved: every cycle each joint is commanded back to its own measured position, so the robot holds whatever pose it is already in and only motors 31 / 33 are driven.

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

The gripper position range is roughly `0.0` (closed) to `5.4` (open). If the values never move, the calibration above did not take effect, or the robot controller is not accepting `rt/lowcmd`.

> ⚠️ The script publishes on `rt/lowcmd`, the whole-body low-level command topic. Make sure nothing else is commanding the robot at the same time, and run it with the robot in a safe state.

> 💡 Requires `unitree_sdk2py` and `logging_mp` in your Python environment.

# ❓ FAQ

1. The calibration program itself needs no DDS: it does not link unitree_sdk2 or CycloneDDS, only libserialport, spdlog/fmt and Boost.program_options. If `cmake ..` fails on a missing package, check that the `apt install` lines above all succeeded.

    The Python verification script in section 3 is a separate matter — it does need `unitree_sdk2py`:

    ```bash
    cd ~
    git clone https://github.com/unitreerobotics/unitree_sdk2_python
    cd unitree_sdk2_python
    pip3 install -e .
    ```

2. The motor is not found:

    ```bash
    unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
    [2025-08-14 09:56:53.595] [info] Available Serial Ports: /dev/ttyUSB1, /dev/ttyUSB0
    [2025-08-14 09:56:54.339] [error] Dex1-1 gripper motor (ID 7) not found.
    # or
    unitree@ubuntu:~/dex1_1_service/build$ sudo ./dex1_1_internal_calibration
    [2025-08-14 09:58:12.010] [info] Available Serial Ports:
    [2025-08-14 09:58:12.010] [error] No supported serial ports found.
    ```

    Possible causes:

    1. The gripper power is not connected, or a cable is loose.
    2. The internal cable between the gripper and the robot's motor bus is not connected properly.
    3. Your gripper is **externally routed** and therefore answers on ID 0 / ID 1 rather than ID 7 — use the `main` branch instead.
