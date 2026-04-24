import argparse
import time
from collections import deque

from unitree_sdk2py.core.channel import ChannelSubscriber, ChannelFactoryInitialize # dds
from unitree_sdk2py.idl.unitree_go.msg.dds_ import MotorStates_, MotorState_        # idl for dex1

import logging_mp
logging_mp.basicConfig(level=logging_mp.INFO)
logger_mp = logging_mp.getLogger(__name__)

kTopicLowState = "rt/lowstate"
kTopicGripperLeftState = "rt/dex1/left/state"
kTopicGripperRightState = "rt/dex1/right/state"

class SimpleFPSMonitor:
    def __init__(self, window_size: int):
        self._times = deque(maxlen=window_size)
        self._last_tick = None
        self._fps = 0.0

    def tick(self):
        now = time.perf_counter_ns()

        if self._last_tick is not None:
            interval_ns = now - self._last_tick
            if interval_ns < 100_000:
                return
            
            self._times.append(interval_ns)
            if len(self._times) == self._times.maxlen:
                rolling_sum = sum(self._times)
                if rolling_sum > 0:
                    self._fps = (len(self._times) * 1_000_000_000.0) / rolling_sum
            else:
                self._fps = 0.0

        self._last_tick = now
    
    def reset(self):
        self._times.clear()
        self._last_tick = None
        self._fps = 0.0

    @property
    def fps(self) -> float:
        """Return 0.0 until the sampling window is fully populated."""
        return self._fps

class GripperStateSubscriber:
    """Subscribe to gripper MotorStates for left and right grippers."""
    def __init__(self, network: str = None):
        logger_mp.info("Initialize GripperStateSubscriber...")
        ChannelFactoryInitialize(0, network)
        self.left_sub = ChannelSubscriber(kTopicGripperLeftState, MotorStates_)
        self.right_sub = ChannelSubscriber(kTopicGripperRightState, MotorStates_)
        # initialize FPS monitors for both grippers
        self.fps_monitor_left = SimpleFPSMonitor(window_size=50)
        self.fps_monitor_right = SimpleFPSMonitor(window_size=50)
        # initialize callbacks
        self.left_sub.Init(self._left_handler, 1)
        self.right_sub.Init(self._right_handler, 1)


    def _left_handler(self, msg: MotorStates_):
        try:
            states = getattr(msg, 'states', None)
            if not states:
                logger_mp.warning('Left gripper: received empty states')
                return
            self.fps_monitor_left.tick()
            logger_mp.info(f'Left  gripper subscribe fps: {self.fps_monitor_left.fps:.3f}')
        except Exception as e:
            logger_mp.exception(f'Error in left gripper handler: {e}')

    def _right_handler(self, msg: MotorStates_):
        try:
            states = getattr(msg, 'states', None)
            if not states:
                logger_mp.warning('Right gripper: received empty states')
                return
            self.fps_monitor_right.tick()
            logger_mp.info(f'Right gripper subscribe fps: {self.fps_monitor_right.fps:.3f}')
        except Exception as e:
            logger_mp.exception(f'Error in right gripper handler: {e}')

    def close(self):
        self.left_sub.Close()
        self.right_sub.Close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Subscribe to Dex1_1 gripper state topics.")
    parser.add_argument(
        "-n",
        "--network",
        default=None,
        help="DDS network interface, for example eth0 or eth1.",
    )
    args = parser.parse_args()

    gripper_state_sub = GripperStateSubscriber(args.network)
    logger_mp.info("Subscribe state...")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger_mp.info("Stopping gripper state subscriber...")
    finally:
        gripper_state_sub.close()
