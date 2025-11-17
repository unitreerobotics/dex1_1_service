#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/idl/go2/MotorStates_.hpp>
#include "dds/Publisher.h"
#include "dds/Subscription.h"
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/common/thread/thread.hpp>
#include "param.h"

#include <chrono>
#include <cmath>
#include <thread>
#include <spdlog/spdlog.h>

int main(int argc, char** argv)
{
    auto vm = param::helper_test(argc, argv);
    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network"].as<std::string>());

    bool use_left  = vm.count("left")  > 0;
    bool use_right = vm.count("right") > 0;
    if (!use_left && !use_right) {
        spdlog::warn("No gripper selected! Please use '--left (or -l)', '--right (or -r)', or both.");
        return 1;
    }

    const float period    = 3.0f; //rad
    const float amplitude = 2.5f; //rad
    const float q_center  = 3.0f; //rad

    std::shared_ptr<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>    left_pub, right_pub;
    std::shared_ptr<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>   left_sub, right_sub;
    float q_left_init = 0.0f, q_right_init = 0.0f;

    // Initialize publishers/subscribers and read initial positions
    if (use_right) {
        right_pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/right/cmd");
        right_sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/right/state");
        right_pub->msg_.cmds().resize(1);
        right_sub->msg_.states().resize(1);
        right_sub->wait_for_connection();

        right_pub->msg_.cmds()[0].mode() = 1;
        right_pub->msg_.cmds()[0].kp()   = 5.0f;
        right_pub->msg_.cmds()[0].kd()   = 0.05f;
        q_right_init = right_sub->msg_.states()[0].q();
        spdlog::info("Right gripper init at q = {:.3f}", q_right_init);
    }

    if (use_left) {
        left_pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/left/cmd");
        left_sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/left/state");
        left_pub->msg_.cmds().resize(1);
        left_sub->msg_.states().resize(1);
        left_sub->wait_for_connection();

        left_pub->msg_.cmds()[0].mode() = 1;
        left_pub->msg_.cmds()[0].kp()   = 5.0f;
        left_pub->msg_.cmds()[0].kd()   = 0.05f;
        q_left_init = left_sub->msg_.states()[0].q();
        spdlog::info("Left gripper init at q = {:.3f}", q_left_init);
    }

    // Compute phase offset so sine starts at the current position
    float phase = 0.0f;
    if (use_right) {
        float arg = (q_right_init - q_center) / amplitude;
        arg = std::clamp(arg, -1.0f, 1.0f);
        phase = std::asin(arg);
    } else if (use_left) {
        float arg = (q_left_init - q_center) / amplitude;
        arg = std::clamp(arg, -1.0f, 1.0f);
        phase = std::asin(arg);
    }

    auto t0 = std::chrono::steady_clock::now() - std::chrono::duration<float>(phase * period / (2.0f * M_PI));

    // Main control loop: sine wave around q_center, starting from current position
    while (true)
    {
        auto t_now = std::chrono::steady_clock::now();
        float t = std::chrono::duration<float>(t_now - t0).count();
        float q = q_center + amplitude * std::sin(2.0f * M_PI * t / period);

        std::cout << "\r";
        if (use_right) {
            std::cout << "R=" << std::setw(6) << std::fixed << std::setprecision(3) << right_sub->msg_.states()[0].q() << " ";
            right_pub->msg_.cmds()[0].q() = q;
            right_pub->unlockAndPublish();
        }
        if (use_left) {
            std::cout << "L=" << std::setw(6) << std::fixed << std::setprecision(3) << left_sub->msg_.states()[0].q() << " ";
            left_pub->msg_.cmds()[0].q() = q;
            left_pub->unlockAndPublish();
        }
        std::cout << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}
