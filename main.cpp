#include "unitreeMotor/unitreeMotor.h"
#include "serialPort/SerialPort.h"

#include "param.h"
#include <cmath>
#include <memory>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h> // fmt::join
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <filesystem>

// The internally-routed Dex1-1 gripper uses motor ID 7,
// unlike the externally-routed one which uses ID 0 (right) / 1 (left).
constexpr int kGripperMotorId = 7;

// Function to list available serial ports
std::vector<std::string> getAvailableSerialPorts() {
    std::vector<std::string> ports;
    constexpr const char* kSupportedPrefixes[] = {
        "/dev/ttyUSB",
        "/dev/ttyCH343USB", // support new serial hub
        "/dev/ttyACM",
    };

    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        std::string path = entry.path().string();
        for (const char* prefix : kSupportedPrefixes) {
            if (path.rfind(prefix, 0) == 0) {
                ports.push_back(path);
                break;
            }
        }
    }
    spdlog::info("Available Serial Ports: {}", fmt::join(ports, ", "));

    return ports;
}

// Probe every available serial port for motor ID 7.
// Both the left and the right internally-routed gripper answer on ID 7, so every
// port that replies is reported back and the caller decides what to do when more
// than one gripper is connected.
std::vector<std::pair<std::string, std::shared_ptr<SerialPort>>> detectGripperMotors(
    const std::vector<std::string>& ports) {
    std::vector<std::pair<std::string, std::shared_ptr<SerialPort>>> found;
    for (const auto& port : ports) {
        // SerialPort throws IOException when the device cannot be opened or
        // configured (busy, no permission, or simply not a motor bus adapter --
        // /dev/ttyACM* is shared with plenty of other USB devices). Skip those
        // ports instead of letting the exception abort the program.
        try {
            auto serial = std::make_shared<SerialPort>(port.c_str());
            MotorCmd cmd;
            MotorData data;
            cmd.motorType = MotorType::M4010;
            cmd.id = kGripperMotorId;
            cmd.mode = queryMotorMode(cmd.motorType, MotorMode::FOC);
            // MotorCmd has a trivial default constructor, so these must be zeroed
            // explicitly: probing in FOC mode with garbage gains would jerk the motor.
            cmd.kp = cmd.kd = cmd.q = cmd.dq = cmd.tau = 0.0f;
            cmd.timeout = 0;
            data.motorType = cmd.motorType;
            usleep(200);
            if (serial->sendRecv(&cmd, &data)) {
                found.emplace_back(port, serial);
            }
        } catch (const std::exception& e) {
            spdlog::debug("Skipping {}: {}", port, e.what());
        }
    }
    return found;
}

int main(int argc, char** argv) {
    param::helper(argc, argv);

    std::vector<std::string> ports = getAvailableSerialPorts();
    if (ports.empty()) {
        spdlog::error("No supported serial ports found.");
        return 1;
    }

    std::vector<std::pair<std::string, std::shared_ptr<SerialPort>>> found;
    for (int attempt = 0; attempt < 3 && found.empty(); ++attempt) {
        found = detectGripperMotors(ports);
        if (found.empty()) {
            usleep(50000);
        }
    }
    if (found.empty()) {
        spdlog::error("Dex1-1 gripper motor (ID {}) not found.", kGripperMotorId);
        return 1;
    }
    if (found.size() > 1) {
        // The left and the right gripper share motor ID 7, so there is no way to
        // tell which one a port belongs to. Refuse rather than calibrate a gripper
        // the user did not mean to calibrate.
        std::vector<std::string> port_names;
        for (const auto& item : found) {
            port_names.push_back(item.first);
        }
        spdlog::error("Found motor ID {} on {} ports: {}", kGripperMotorId, found.size(),
                      fmt::join(port_names, ", "));
        spdlog::error("Both the left and the right gripper use ID {}, so they cannot be told apart. "
                      "Please connect only ONE Dex1-1 gripper at a time and run this program again.",
                      kGripperMotorId);
        return 1;
    }

    const std::string& port_name = found.front().first;
    const std::shared_ptr<SerialPort>& serial = found.front().second;

    spdlog::info("========== Motor Calibration ==========");
    spdlog::info("  - Motor ID: {}, \t Port: {}", kGripperMotorId, port_name);
    spdlog::info("Please manually close the gripper tightly. \n \t\t\t\t Then press 's' + Enter to calibrate, or any other key to skip.");

    char key;
    std::cout << "> ";
    if (!(std::cin >> key)) {
        // stdin closed or redirected: never calibrate on an unread answer.
        spdlog::error("No input available. Calibration aborted.");
        return 1;
    }
    if (key == 's' || key == 'S') {
        spdlog::info("Calibrating motor {}...", kGripperMotorId);
        bool res = serial->calibration(MotorType::M4010, kGripperMotorId, 0.0f, 322 * (M_PI / 180.));
        if (res) {
            spdlog::info("Motor {} calibration successful.", kGripperMotorId);
        } else {
            spdlog::error("Motor {} calibration failed.", kGripperMotorId);
            return 1;
        }
    } else {
        spdlog::info("Skipped calibration for motor {}.", kGripperMotorId);
    }

    spdlog::info("Calibration process completed.");
    return 0;
}
