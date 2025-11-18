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
#include <atomic>
#include <iostream>
#include <iomanip>
#include <limits>
#include <spdlog/spdlog.h>

std::atomic<float> cmd_q(0.0f);
std::atomic<bool> running(true);

std::atomic<bool> measuring_left(false), measuring_right(false);
std::chrono::steady_clock::time_point start_left, start_right;

void input_thread()
{
    while (running) {
        float tmp;
        std::cout << "\nplease input target q (rad) for grippers (9999 to exit): ";
        if (!(std::cin >> tmp)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (tmp == 9999.0f) {
            running = false;
            break;
        }
        cmd_q = tmp;

        measuring_left = true;
        measuring_right = true;
        start_left = std::chrono::steady_clock::now();
        start_right = std::chrono::steady_clock::now();
    }
}

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
    std::cout<<"start"<<std::endl;

    std::shared_ptr<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>    left_pub, right_pub;
    std::shared_ptr<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>   left_sub, right_sub;


    if (use_right) {
        right_sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/right/state");
        right_sub->msg_.states().resize(1);
        right_sub->wait_for_connection();

        right_pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/right/cmd");
        right_pub->msg_.cmds().resize(1);
        right_pub->msg_.cmds()[0].mode() = 1;
        right_pub->msg_.cmds()[0].kp()   = 5.0f;
        right_pub->msg_.cmds()[0].kd()   = 0.05f;
        float q_init = right_sub->msg_.states()[0].q();
        spdlog::info("Right gripper init at q = {:.3f}", q_init);
    }


    if (use_left) {
        left_sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/left/state");
        left_sub->msg_.states().resize(1);
        left_sub->wait_for_connection();

        left_pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/left/cmd");
        left_pub->msg_.cmds().resize(1);
        left_pub->msg_.cmds()[0].mode() = 1;
        left_pub->msg_.cmds()[0].kp()   = 5.0f;
        left_pub->msg_.cmds()[0].kd()   = 0.05f;
        float q_init = left_sub->msg_.states()[0].q();
        spdlog::info("Left gripper init at q = {:.3f}", q_init);
    }

    std::thread t(input_thread);


    while (running) {
        float target = cmd_q.load();
        std::cout << "\r";

        if (use_right) {
            float state_q = right_sub->msg_.states()[0].q();

            right_pub->msg_.cmds()[0].q() = target;
            right_pub->unlockAndPublish();

            if (measuring_right) {
                float error = std::fabs(state_q - target);
                if (error < 0.05f) {
                    auto end_time = std::chrono::steady_clock::now();
                    double elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_right).count();
                    std::cout << "\n Right gripper reached target position in " << elapsed_ms << " ms\n";
                    measuring_right = false;
                }
            }
        }

        if (use_left) {
            float state_q = left_sub->msg_.states()[0].q();
            std::cout << "L=" << std::setw(6) << std::fixed << std::setprecision(3) << state_q << " ";
            left_pub->msg_.cmds()[0].q() = target;
            left_pub->unlockAndPublish();

            if (measuring_left) {
                float error = std::fabs(state_q - target);
                if (error < 0.01f) {
                    auto end_time = std::chrono::steady_clock::now();
                    double elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_left).count();
                    std::cout << "\n Left gripper reached target position in " << elapsed_ms << " ms\n";
                    measuring_left = false;
                }
            }
        }

        std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (t.joinable()) t.join();
    spdlog::info("Test program exited cleanly.");
    return 0;
}