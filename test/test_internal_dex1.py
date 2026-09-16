#!/usr/bin/env python3
"""Simple test script for G1_29_Arm_Internal_Dex1_Controller.

Only the two internally-routed Dex1-1 grippers (motor 31 / 33) are driven.
The arms are held wherever they currently are: every cycle their command is
taken straight from the latest lowstate, so no homing motion is performed.
"""
import logging_mp
logging_mp.basicConfig(level=logging_mp.INFO)
logger_mp = logging_mp.getLogger(__name__)

import argparse
import time
from multiprocessing import Value

import numpy as np
import threading
from enum import IntEnum

from unitree_sdk2py.core.channel import ChannelPublisher, ChannelSubscriber, ChannelFactoryInitialize # dds
from unitree_sdk2py.idl.unitree_hg.msg.dds_ import ( LowCmd_  as hg_LowCmd, LowState_ as hg_LowState) # idl for g1, h1_2
from unitree_sdk2py.idl.default import unitree_hg_msg_dds__LowCmd_
from unitree_sdk2py.utils.crc import CRC

kTopicLowCommand_Debug  = "rt/lowcmd"
kTopicLowState = "rt/lowstate"
G1_29_Num_Motors = 35
G1_29_Body_Motors = 29

class DataBuffer:
    def __init__(self):
        self.data = None
        self.lock = threading.Lock()

    def GetData(self):
        with self.lock:
            return self.data

    def SetData(self, data):
        with self.lock:
            self.data = data

class G1_29_JointArmIndex(IntEnum):
    # Left arm
    kLeftShoulderPitch = 15
    kLeftShoulderRoll = 16
    kLeftShoulderYaw = 17
    kLeftElbow = 18
    kLeftWristRoll = 19
    kLeftWristPitch = 20
    kLeftWristyaw = 21

    # Right arm
    kRightShoulderPitch = 22
    kRightShoulderRoll = 23
    kRightShoulderYaw = 24
    kRightElbow = 25
    kRightWristRoll = 26
    kRightWristPitch = 27
    kRightWristYaw = 28

class G1_29_JointIndex(IntEnum):
    # Left leg
    kLeftHipPitch = 0
    kLeftHipRoll = 1
    kLeftHipYaw = 2
    kLeftKnee = 3
    kLeftAnklePitch = 4
    kLeftAnkleRoll = 5

    # Right leg
    kRightHipPitch = 6
    kRightHipRoll = 7
    kRightHipYaw = 8
    kRightKnee = 9
    kRightAnklePitch = 10
    kRightAnkleRoll = 11

    kWaistYaw = 12
    kWaistRoll = 13
    kWaistPitch = 14

    # Left arm
    kLeftShoulderPitch = 15
    kLeftShoulderRoll = 16
    kLeftShoulderYaw = 17
    kLeftElbow = 18
    kLeftWristRoll = 19
    kLeftWristPitch = 20
    kLeftWristyaw = 21

    # Right arm
    kRightShoulderPitch = 22
    kRightShoulderRoll = 23
    kRightShoulderYaw = 24
    kRightElbow = 25
    kRightWristRoll = 26
    kRightWristPitch = 27
    kRightWristYaw = 28

    # not used
    kNotUsedJoint0 = 29
    kNotUsedJoint1 = 30
    kNotUsedJoint2 = 31
    kNotUsedJoint3 = 32
    kNotUsedJoint4 = 33
    kNotUsedJoint5 = 34

class G1_29_Internal_Dex1_JointIndex(IntEnum):
    kLeftDex1_1 = 31
    kRightDex1_1 = 33

class G1_29_Arm_Internal_Dex1_Controller:
    def __init__(self, left_gripper_value_in, right_gripper_value_in, dual_gripper_data_lock = None, dual_gripper_state_out = None, dual_gripper_action_out = None,
                 xr_motion_data_ready_in = None):
        logger_mp.info("Initialize G1_29_Arm_Internal_Dex1_Controller...")

        self.left_gripper_value_in = left_gripper_value_in
        self.right_gripper_value_in = right_gripper_value_in
        self.xr_motion_data_ready_in = xr_motion_data_ready_in
        self.control_dt = 1.0 / 250.0
        self.left_index = G1_29_Internal_Dex1_JointIndex.kLeftDex1_1.value
        self.right_index = G1_29_Internal_Dex1_JointIndex.kRightDex1_1.value
        self.dual_gripper_data_lock = dual_gripper_data_lock
        self.dual_gripper_state_out = dual_gripper_state_out
        self.dual_gripper_action_out = dual_gripper_action_out

        self.gripper_q_target = np.zeros(2)
        self.running = True

        self.kp_high = 300.0
        self.kd_high = 3.0
        self.kp_low = 80.0
        self.kd_low = 3.0
        self.kp_wrist = 40.0
        self.kd_wrist = 1.5
        self.gripper_kp = 5.0
        self.gripper_kd = 0.05
        self.gripper_delta = 0.18

        self.lowcmd_publisher = ChannelPublisher(kTopicLowCommand_Debug, hg_LowCmd)
        self.lowcmd_publisher.Init()
        self.lowstate_subscriber = ChannelSubscriber(kTopicLowState, hg_LowState)
        self.lowstate_subscriber.Init()
        self.lowstate_buffer = DataBuffer()

        self.subscribe_thread = threading.Thread(target=self._subscribe_motor_state)
        self.subscribe_thread.daemon = True
        self.subscribe_thread.start()

        while not self.lowstate_buffer.GetData():
            time.sleep(0.1)
            logger_mp.warning("[G1_29_Arm_Internal_Dex1_Controller] Waiting to subscribe dds...")
        logger_mp.info("[G1_29_Arm_Internal_Dex1_Controller] Subscribe dds ok.")

        self.crc = CRC()
        self.ctrl_lock = threading.Lock()
        self.msg = unitree_hg_msg_dds__LowCmd_()
        self.msg.mode_pr = 0
        self.msg.mode_machine = self.lowstate_buffer.GetData().mode_machine

        self.all_motor_q = self.get_current_motor_q()
        self.gripper_q_target = self.get_current_dual_gripper_q()
        logger_mp.info(f"Current dual gripper q: {self.gripper_q_target}")

        arm_indices = set(member.value for member in G1_29_JointArmIndex)
        gripper_indices = {self.left_index, self.right_index}
        for id in G1_29_JointIndex:
            cmd = self.msg.motor_cmd[id]
            cmd.q = self.all_motor_q[id]
            cmd.dq = 0.0
            cmd.tau = 0.0
            if id.value in gripper_indices:
                cmd.mode = 1
                cmd.kp = self.gripper_kp
                cmd.kd = self.gripper_kd
            elif id.value >= G1_29_Body_Motors:
                cmd.mode = 0
                cmd.kp = 0.0
                cmd.kd = 0.0
            elif id.value in arm_indices:
                cmd.mode = 1
                cmd.kp = self.kp_wrist if self._Is_wrist_motor(id) else self.kp_low
                cmd.kd = self.kd_wrist if self._Is_wrist_motor(id) else self.kd_low
            else:
                cmd.mode = 1
                cmd.kp = self.kp_low if self._Is_weak_motor(id) else self.kp_high
                cmd.kd = self.kd_low if self._Is_weak_motor(id) else self.kd_high

        self.publish_thread = threading.Thread(target=self._ctrl_motor_state)
        self.publish_thread.daemon = True
        self.publish_thread.start()

        logger_mp.info("Initialize G1_29_Arm_Internal_Dex1_Controller OK!")

    def _subscribe_motor_state(self):
        while self.running:
            msg = self.lowstate_subscriber.Read()
            if msg is not None:
                self.lowstate_buffer.SetData(msg)
            time.sleep(0.002)

    def _ctrl_motor_state(self):
        THUMB_INDEX_DISTANCE_MIN = 5.0
        THUMB_INDEX_DISTANCE_MAX = 7.0
        LEFT_MAPPED_MIN = 0.0
        RIGHT_MAPPED_MIN = 0.0
        LEFT_MAPPED_MAX = 5.40
        RIGHT_MAPPED_MAX = 5.40
        logger_mp.info(f"LEFT_MAPPED_MAX = {LEFT_MAPPED_MAX}, RIGHT_MAPPED_MAX = {RIGHT_MAPPED_MAX}")

        while self.running:
            start_time = time.time()
            lowstate = self.lowstate_buffer.GetData()
            with self.left_gripper_value_in.get_lock():
                left_gripper_value = self.left_gripper_value_in.value
            with self.right_gripper_value_in.get_lock():
                right_gripper_value = self.right_gripper_value_in.value
            if self.xr_motion_data_ready_in is not None:
                with self.xr_motion_data_ready_in.get_lock():
                    xr_motion_data_ready = self.xr_motion_data_ready_in.value
            else:
                xr_motion_data_ready = True

            # Hold every joint, arms included, at its measured position. Only the
            # gripper commands below deviate from the current state.
            for id in G1_29_JointIndex:
                self.msg.motor_cmd[id].q = lowstate.motor_state[id].q
                self.msg.motor_cmd[id].dq = 0.0
                self.msg.motor_cmd[id].tau = 0.0

            gripper_state = self.get_current_dual_gripper_q()
            if xr_motion_data_ready:
                left_target_action = np.interp(left_gripper_value, [THUMB_INDEX_DISTANCE_MIN, THUMB_INDEX_DISTANCE_MAX], [LEFT_MAPPED_MIN, LEFT_MAPPED_MAX])
                right_target_action = np.interp(right_gripper_value, [THUMB_INDEX_DISTANCE_MIN, THUMB_INDEX_DISTANCE_MAX], [RIGHT_MAPPED_MIN, RIGHT_MAPPED_MAX])
                with self.ctrl_lock:
                    self.gripper_q_target = np.array([left_target_action, right_target_action])
            else:
                with self.ctrl_lock:
                    self.gripper_q_target = gripper_state.copy()
            with self.ctrl_lock:
                gripper_q_target = self.gripper_q_target.copy()
            gripper_q_cmd = np.clip(gripper_q_target, gripper_state - self.gripper_delta, gripper_state + self.gripper_delta)

            for idx, id in enumerate((self.left_index, self.right_index)):
                self.msg.motor_cmd[id].mode = 1
                self.msg.motor_cmd[id].q = gripper_q_cmd[idx]
                self.msg.motor_cmd[id].dq = 0.0
                self.msg.motor_cmd[id].tau = 0.0
                self.msg.motor_cmd[id].kp = self.gripper_kp
                self.msg.motor_cmd[id].kd = self.gripper_kd

            if self.dual_gripper_state_out is not None and self.dual_gripper_action_out is not None and self.dual_gripper_data_lock is not None:
                with self.dual_gripper_data_lock:
                    self.dual_gripper_state_out[:] = gripper_state
                    self.dual_gripper_action_out[:] = gripper_q_cmd

            self.msg.crc = self.crc.Crc(self.msg)
            self.lowcmd_publisher.Write(self.msg)

            sleep_time = max(0.0, self.control_dt - (time.time() - start_time))
            time.sleep(sleep_time)

    def get_current_motor_q(self):
        data = self.lowstate_buffer.GetData()
        return np.array([data.motor_state[id].q for id in range(G1_29_Num_Motors)])

    def get_current_dual_gripper_q(self):
        data = self.lowstate_buffer.GetData()
        return np.array([data.motor_state[self.left_index].q, data.motor_state[self.right_index].q])

    def _Is_weak_motor(self, motor_index):
        weak_motors = [
            G1_29_JointIndex.kLeftAnklePitch.value,
            G1_29_JointIndex.kRightAnklePitch.value,
            G1_29_JointIndex.kLeftShoulderPitch.value,
            G1_29_JointIndex.kLeftShoulderRoll.value,
            G1_29_JointIndex.kLeftShoulderYaw.value,
            G1_29_JointIndex.kLeftElbow.value,
            G1_29_JointIndex.kRightShoulderPitch.value,
            G1_29_JointIndex.kRightShoulderRoll.value,
            G1_29_JointIndex.kRightShoulderYaw.value,
            G1_29_JointIndex.kRightElbow.value,
        ]
        return motor_index.value in weak_motors

    def _Is_wrist_motor(self, motor_index):
        wrist_motors = [
            G1_29_JointIndex.kLeftWristRoll.value,
            G1_29_JointIndex.kLeftWristPitch.value,
            G1_29_JointIndex.kLeftWristyaw.value,
            G1_29_JointIndex.kRightWristRoll.value,
            G1_29_JointIndex.kRightWristPitch.value,
            G1_29_JointIndex.kRightWristYaw.value,
        ]
        return motor_index.value in wrist_motors

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--network-interface", required=True, help="CycloneDDS interface, e.g. enp132s0")
    args = parser.parse_args()

    logger_mp.info(f"initializing DDS domain 0 --network-interface={args.network_interface}")
    ChannelFactoryInitialize(0, args.network_interface)

    left_gripper_value = Value('d', 0.0, lock=True)
    right_gripper_value = Value('d', 0.0, lock=True)
    ctrl = G1_29_Arm_Internal_Dex1_Controller(left_gripper_value, right_gripper_value)

    XR_INPUT_MIN = 0.5
    XR_INPUT_MAX = 8.0

    logger_mp.info("\nG1_29 internal Dex1 verifier:")
    logger_mp.info(f"-----------------------------------")
    def run_phase(label, xr_input_value):
        with left_gripper_value.get_lock():
            left_gripper_value.value = xr_input_value
        with right_gripper_value.get_lock():
            right_gripper_value.value = xr_input_value
        deadline = time.time() + 8.0
        next_print = 0.0
        while time.time() < deadline:
            q = ctrl.get_current_dual_gripper_q()
            if time.time() >= next_print:
                logger_mp.info(f"  {label}: left={q[0]:.3f} right={q[1]:.3f}")
                next_print = time.time() + 0.2
            time.sleep(0.01)

    try:
        run_phase("open", XR_INPUT_MAX)
        time.sleep(0.5)
        run_phase("close", XR_INPUT_MIN)
        time.sleep(0.5)
    except KeyboardInterrupt:
        logger_mp.info("\ninterrupted")
    finally:
        # Stop publishing before the process exits, so the last rt/lowcmd on the
        # wire is a deliberate one rather than whatever a killed thread left.
        ctrl.running = False
        ctrl.publish_thread.join(timeout=1.0)
        ctrl.subscribe_thread.join(timeout=1.0)


if __name__ == "__main__":
    raise SystemExit(main())
