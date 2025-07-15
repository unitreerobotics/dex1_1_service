#ifndef _UT_ROBOT_SUBSCRIPTION_H_
#define _UT_ROBOT_SUBSCRIPTION_H_

#include <unitree/robot/channel/channel_subscriber.hpp>
#include <mutex>

namespace unitree
{
namespace robot
{

template <typename MessageType>
class SubscriptionBase
{
public:
  using MsgType = MessageType;
  using SharedPtr = std::shared_ptr<SubscriptionBase<MsgType>>;
  
  SubscriptionBase(const std::string& topic)
  {
    last_update_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(timeout_ms_);
    sub_ = std::make_shared<unitree::robot::ChannelSubscriber<MessageType>>(topic);
    sub_->InitChannel([this](const void *msg){
      last_update_time_ = std::chrono::steady_clock::now();
      std::lock_guard<std::mutex> lock(mutex_);
      msg_ = *(const MessageType*)msg;
      post_communication();
    });
  }

  void set_timeout_ms(uint32_t timeout_ms) { timeout_ms_ = timeout_ms; }

  bool isTimeout() {
    auto now = std::chrono::steady_clock::now();
    auto elasped_time = now - last_update_time_;
    return elasped_time > std::chrono::milliseconds(timeout_ms_);
  }

  void wait_for_connection() {
    while (isTimeout()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    sleep(1); // wait for the connection to be stable
  }

  MessageType msg_;
  std::mutex mutex_;
protected:
  virtual void post_communication() {} // something after receiving message

  uint32_t timeout_ms_{1000};
  unitree::robot::ChannelSubscriberPtr<MessageType> sub_;
  std::chrono::steady_clock::time_point last_update_time_;
};


}; // namespace robot
}; // namespace unitree

#endif // _UT_ROBOT_SUBSCRIPTION_H_