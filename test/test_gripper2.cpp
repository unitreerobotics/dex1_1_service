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
#include <mutex>
#include <sstream>
#include <spdlog/spdlog.h>

std::atomic<float> cmd_q(0.0f);
std::atomic<bool> running(true);

std::atomic<bool> measuring(false);
std::chrono::steady_clock::time_point start_time;
std::atomic<float> elapsed_ms(-1.0f);

std::atomic<int> stable_count(0);
const int REQUIRED_STABLE = 3;

std::mutex io_mutex;

static void clear_line_tail() {
    std::cout << "\033[K";
}

void input_thread()
{
    while (running) {
        float tmp;

        {
            std::lock_guard<std::mutex> lk(io_mutex);
            std::cout << "\033[3;1H";
            std::cout << "please input target angle (rad) [9999 to quit]: ";
            clear_line_tail();
            std::cout.flush();
        }

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
        measuring = true;
        elapsed_ms = -1.0f;
        stable_count = 0;
        start_time = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lk(io_mutex);
            std::cout << "\033[3;1H";
            std::cout << "please input target angle (rad) [9999 to quit]: ";
            clear_line_tail();
            std::cout.flush();
        }
    }
}


void print_status_labelled(bool use_left, float q, float elapsed) {
    std::lock_guard<std::mutex> lk(io_mutex);

    std::cout << "\033[s";

    std::cout << "\033[1;1H";
    if (use_left) {
        std::cout << "Left:  " << std::fixed << std::setprecision(3) << q;
    } else {
        std::cout << "Right: " << std::fixed << std::setprecision(3) << q;
    }
    clear_line_tail();

    std::cout << "\033[2;1H";
    std::cout << "Time to target (ms): ";
    if (elapsed >= 0.0f) {
        std::cout << static_cast<int>(elapsed);
    } else {
        std::cout << "   ";
    }
    clear_line_tail();

    std::cout << "\033[u";
    std::cout.flush();
}

int main(int argc, char** argv)
{
    auto vm = param::helper_test(argc, argv);
    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network"].as<std::string>());

    bool use_left  = vm.count("left")  > 0;
    bool use_right = vm.count("right") > 0;

    if (use_left == use_right) {
        spdlog::warn("Please specify either --left or --right (but not both).");
        return 1;
    }

    std::shared_ptr<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>> pub;
    std::shared_ptr<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>> sub;

    if (use_left) {
        sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/left/state");
        sub->msg_.states().resize(1);
        sub->wait_for_connection();

        pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/left/cmd");
        pub->msg_.cmds().resize(1);
        pub->msg_.cmds()[0].mode() = 1;
        pub->msg_.cmds()[0].kp()   = 5.0f;
        pub->msg_.cmds()[0].kd()   = 0.05f;
    } else {
        sub = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/dex1/right/state");
        sub->msg_.states().resize(1);
        sub->wait_for_connection();

        pub = std::make_shared<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/dex1/right/cmd");
        pub->msg_.cmds().resize(1);
        pub->msg_.cmds()[0].mode() = 1;
        pub->msg_.cmds()[0].kp()   = 5.0f;
        pub->msg_.cmds()[0].kd()   = 0.05f;
    }

    std::thread t(input_thread);

    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "\033[2J";
        std::cout << "\033[1;1H";
        std::cout.flush();
    }

    while (running) {
        float target = cmd_q.load();
        float q = sub->msg_.states()[0].q();

        pub->msg_.cmds()[0].q() = target;
        pub->unlockAndPublish();

        bool within = false;
        within = std::fabs(q - target) < 0.05f;

        if (measuring.load()) {
            if (within) {
                if (stable_count == 0) {
                    auto end = std::chrono::steady_clock::now();
                    float ms = static_cast<float>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time).count());
                    elapsed_ms = ms;
                    measuring = false;
                }
                stable_count++;
            } else {
                stable_count = 0;
            }
        }        

        if (measuring.load() && stable_count.load() == 0 && within) {
            auto end = std::chrono::steady_clock::now();
            float ms = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time).count());
            if (ms < 10.0f) {
                elapsed_ms = ms;
                measuring = false;
            }
        }

        float em = elapsed_ms.load();
        print_status_labelled(use_left, q, em);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (t.joinable()) t.join();

    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "\033[4;1H";
        std::cout << "\nExiting...\n";
        std::cout.flush();
    }

    return 0;
}
