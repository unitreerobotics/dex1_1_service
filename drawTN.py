import numpy as np
import matplotlib.pyplot as plt

# =========================
# Unitree W4010-25 parameters
# =========================
X1 = 15.3      # Knee point speed (rad/s)
X2 = 24.76     # No-load speed (rad/s)
Y1 = 4.8       # Forward peak torque (Nm)
Y2 = 8.6       # Reverse peak torque magnitude (Nm)
Fs = 0.6       # Static friction torque (Nm)
Fd = 0.06      # Dynamic friction coefficient (Nm/(rad/s))
Va = 0.01      # Static friction activation velocity (rad/s)

# =========================
# Velocity range (allow negative speed)
# =========================
omega = np.linspace(-30, 30, 800)

# =========================
# Torque-speed curves
# =========================
# Forward torque curve (ω ≥ 0)
T_forward = np.where(
    omega >= 0,
    np.where(
        omega < X1,
        Y1,
        np.clip(Y1 * (X2 - omega) / (X2 - X1), 0, Y1)
    ),
    0.0
)

# Reverse torque curve (ω < 0)
T_reverse = np.where(
    omega < 0,
    -np.where(
        -omega < X1,
        Y2,
        np.clip(Y2 * (X2 - (-omega)) / (X2 - X1), 0, Y2)
    ),
    0.0
)

# Combine motor torque
T_motor = T_forward + T_reverse

# =========================
# Friction model (directional)
# =========================
T_friction = np.sign(omega) * (Fs * np.tanh(np.abs(omega) / Va) + Fd * np.abs(omega))

# =========================
# Limit friction so it cannot exceed motor torque (prevents torque reversal)
# =========================
T_friction_limited = np.zeros_like(T_friction)
for i in range(len(omega)):
    if T_motor[i] > 0:
        T_friction_limited[i] = min(T_friction[i], T_motor[i])
    elif T_motor[i] < 0:
        T_friction_limited[i] = max(T_friction[i], T_motor[i])
    else:
        T_friction_limited[i] = 0.0

# Net output torque
T_net = T_motor - T_friction_limited

# =========================
# Plotting
# =========================
plt.figure(figsize=(10, 6))

plt.plot(omega, T_motor, label='Motor Torque (No Friction)', linewidth=2)
plt.plot(omega, T_net, '--', label='Net Torque (With Friction Limited)', linewidth=2)

# Limits
plt.axhline(0, color='black', linewidth=1)
plt.axvline(0, color='gray', linestyle='--', linewidth=1)

# Mark key points
plt.scatter([X1, -X1, X2, -X2], [Y1, -Y2, 0, 0], color='red')
plt.text(X1+0.3, Y1, "X1/Y1", color="red")
plt.text(-X1-4, -Y2, "X1/Y2", color="red")
plt.text(X2+0.3, 0.2, "X2", color="black")
plt.text(-X2-3, 0.2, "-X2", color="black")

plt.xlabel("Speed ω (rad/s)")
plt.ylabel("Torque T (N·m)")
plt.title("Unitree W4010-25 Motor Torque-Speed (T-N) Curve with Friction")
plt.grid(True)
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0.)
plt.tight_layout()
plt.xlim(-30, 30)
plt.ylim(-Y2 - 2, Y1 + 2)
plt.savefig("gripper_TN.png", dpi=300)
plt.show()