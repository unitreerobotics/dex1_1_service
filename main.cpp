#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/idl/go2/MotorStates_.hpp>
#include <unitree/common/thread/thread.hpp>
#include "dds/Publisher.h"
#include "dds/Subscription.h"

#include "unitreeMotor/unitreeMotor.h"
#include "serialPort/SerialPort.h"

#include "param.h"
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>


std::vector<std::string> getAvailableSerialPorts() {
    std::vector<std::string> ports;

    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        std::string path = entry.path().string();
        if (path.rfind("/dev/ttyUSB", 0) == 0) {  // Only include ttyUSB*
            ports.push_back(path);
        }
    }
    spdlog::info("Available Serial Ports: {}", fmt::join(ports, ", "));

    return ports;
}


class MotorUnit {
public:
    MotorUnit(int id, std::shared_ptr<SerialPort> serial, const std::string& cmdTopic, const std::string& stateTopic)
        : id_(id), serial_(serial), cmdTopic_(cmdTopic), stateTopic_(stateTopic) {

        cmd_.id = id_;
        cmd_.motorType = MotorType::M4010;
        cmd_.mode = queryMotorMode(cmd_.motorType, MotorMode::BRAKE);
        state_.motorType = cmd_.motorType;
        gear_ratio_ = queryGearRatio(cmd_.motorType);

        sub_ = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorCmds_>>(cmdTopic_);
        pub_ = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorStates_>>(stateTopic_);
        sub_->msg_.cmds().resize(1);
        pub_->msg_.states().resize(1);

        thread_ = std::make_unique<unitree::common::RecurrentThread>(2000, [&]() {
            this->loop();
        });
    }

private:
    void loop() {
        if (sub_->isTimeout()) {
            cmd_.mode = queryMotorMode(cmd_.motorType, MotorMode::BRAKE);
            cmd_.kp = cmd_.kd = cmd_.q = cmd_.dq = cmd_.tau = 0.0f;
            cmd_.timeout = 1;
        } 
        else {
            auto& msg = sub_->msg_.cmds()[0];
            cmd_.mode = queryMotorMode(cmd_.motorType, MotorMode::FOC);
            cmd_.kp = msg.kp() / (gear_ratio_ * gear_ratio_);
            cmd_.kd = msg.kd() / (gear_ratio_ * gear_ratio_);
            cmd_.q = msg.q() * gear_ratio_;
            cmd_.dq = msg.dq() * gear_ratio_;
            cmd_.tau = msg.tau() / gear_ratio_;
            cmd_.timeout = 0;
        }

        serial_->sendRecv(&cmd_, &state_);
        if (pub_->trylock()) {
            pub_->msg_.states()[0].q() = state_.q / gear_ratio_;
            pub_->msg_.states()[0].dq() = state_.dq / gear_ratio_;
            pub_->msg_.states()[0].tau_est() = state_.tau * gear_ratio_;
            pub_->unlockAndPublish();
        }
    }

    int id_;
    float gear_ratio_;
    std::string cmdTopic_, stateTopic_;
    std::shared_ptr<SerialPort> serial_;
    MotorCmd cmd_;
    MotorData state_;
    std::shared_ptr<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorCmds_>> sub_;
    std::shared_ptr<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorStates_>> pub_;
    std::unique_ptr<unitree::common::RecurrentThread> thread_;
};

class Dex1GripperServer {
public:
    Dex1GripperServer(const std::vector<std::string>& ports) {
        bool found = false;
        for (int attempt = 0; attempt < 3 && !found; ++attempt) {
            detectMotors(ports);
            found = !motors_.empty();
            if (!found) {
                usleep(50000);
            }
        }
        if (!found) {
            spdlog::error("Motors not found after multiple attempts.");
            exit(1);
        }
    }

    void runDDS() {
        // Initialize DDS for each detected motor
        for (auto& [id, motor_info] : motors_) {
            std::string side = (id == 0) ? "right" : "left";
            std::string cmdTopic = "rt/dex1/" + side + "/cmd";
            std::string stateTopic = "rt/dex1/" + side + "/state";
            motor_info.unit = std::make_unique<MotorUnit>(id, motor_info.serial, cmdTopic, stateTopic);
        }
    }

    void calibrate() {
        char key;
        int total = motors_.size();
        int index = 1;
        for (auto& [id, motor_info] : motors_) {
            std::string side = (id == 0) ? "right" : "left";
            spdlog::info("========== Motor Calibration (Motor {} (index) of {} (total)) ==========", index, total);
            spdlog::info("  - Motor ID: {}, \t Side: {}, \t Port: {}", id, side, motor_info.port_name);
            spdlog::info("Please manually close the gripper tightly. \n \t\t\t\t Then press 's' + Enter to calibrate, or any other key to skip.");
            std::cout << "> ";
            std::cin >> key;
            if (key == 's'|| key == 'S') {
                spdlog::info("Calibrating motor {}...", id);
                bool res = motor_info.serial->calibration(MotorType::M4010, id, 0.0f, 322 * (M_PI / 180.));
                if (res)
                    spdlog::info("Motor {} calibration successful.", id);
                else
                    spdlog::error("Motor {} calibration failed.", id);
            } else {
                spdlog::info("Skipped calibration for motor {}.", id);
            }
            ++index;
        }
        spdlog::info("Calibration process completed.");
        exit(0);
    }

private:
    struct MotorInfo {
        std::shared_ptr<SerialPort> serial;
        std::string port_name;
        std::unique_ptr<MotorUnit> unit;
    };

    std::map<int, MotorInfo> motors_;

    void detectMotors(const std::vector<std::string>& ports) {
        // ===================== Silence begin =====================
        // Temporarily redirect stdout to /dev/null
        // This is only to suppress unwanted console output from serial->sendRecv()
        int saved_stdout = dup(STDOUT_FILENO);
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, STDOUT_FILENO);
        close(null_fd);
        // ===================== Silence begin =====================

        for (const auto& port : ports) {
            auto serial = std::make_shared<SerialPort>(port.c_str());
            for (int id = 0; id <= 1; ++id) {
                MotorCmd cmd;
                MotorData data;
                cmd.motorType = MotorType::M4010;
                cmd.id = id;
                cmd.mode = queryMotorMode(cmd.motorType, MotorMode::FOC);
                data.motorType = cmd.motorType;

                if (serial->sendRecv(&cmd, &data)) {
                    motors_[id] = { serial, port, nullptr };
                }
                usleep(200);
            }
        }

        // ===================== Silence end =====================
        // Restore original stdout
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        // ===================== Silence end =====================
        
        if (!motors_.empty()) {
            spdlog::info("Detected motors:");
            for (const auto& [id, motor_info] : motors_) {
                std::string side = (id == 0) ? "right" : "left";
                std::string cmdTopic = "rt/dex1/" + side + "/cmd";
                std::string stateTopic = "rt/dex1/" + side + "/state";
                spdlog::info("  - Motor ID: {} \t Side: {} \t Port: {} \t cmdTopic: {} \t stateTopic: {}", id, side, motor_info.port_name, cmdTopic, stateTopic);
            }
        }
    }
};

int main(int argc, char** argv) {
    auto vm = param::helper(argc, argv);
    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network"].as<std::string>());

    std::vector<std::string> ports = getAvailableSerialPorts();
    if (ports.empty()) {
        spdlog::warn("No ttyUSB serial ports found.");
        return 0;
    }

    Dex1GripperServer server(ports);

    if (vm.count("calibration")) {
        server.calibrate();
        return 0;
    }

    server.runDDS();
    spdlog::info("Dex1-1 Gripper Server started.");

    while (true) sleep(1);
    return 0;
}