#!/usr/bin/env python3
import argparse
import subprocess
import time
import matplotlib
matplotlib.use("TkAgg")  # Asegura compatibilidad con animación
import matplotlib.pyplot as plt
import matplotlib.animation as animation

DEVICE = "/dev/SdC_cdd"
SIGNALS = {
    0: {"name": "Canal 0 — AIN0 (P9.39)", "unit": "mV"},
    1: {"name": "Canal 1 — AIN1 (P9.40)", "unit": "mV"},
}
MAX_POINTS = 60

BBB_HOST = "debian@10.42.0.228"

current_signal = 0
values = []
timestamps = []
start_time = time.time()


def _ssh(cmd):
    result = subprocess.run(
        ["ssh", "-i", "~/.ssh/id_rsa_bbb", "-o", "BatchMode=yes", BBB_HOST, cmd],
        capture_output=True, text=True, timeout=5
    )
    return result.stdout.strip()


def select_signal(sig_id):
    _ssh(f"echo {sig_id} > {DEVICE}")


def read_value():
    return float(_ssh(f"cat {DEVICE}"))


def reset_buffers():
    global values, timestamps, start_time
    values = []
    timestamps = []
    start_time = time.time()


def update(frame):
    global current_signal

    try:
        val = read_value()
    except Exception as e:
        print(f"Error leyendo device: {e}")
        return line,

    elapsed = time.time() - start_time
    values.append(val)
    timestamps.append(elapsed)

    if len(values) > MAX_POINTS:
        values.pop(0)
        timestamps.pop(0)

    line.set_xdata(values)
    line.set_ydata(timestamps)

    ax.set_xlim(0, 1800)
    if timestamps:
        ax.set_ylim(max(0, timestamps[-1] - MAX_POINTS), timestamps[-1] + 1)

    return line,


def on_key(event):
    global current_signal
    if event.key == "s":
        current_signal = 1 - current_signal
        select_signal(current_signal)
        reset_buffers()
        sig = SIGNALS[current_signal]
        ax.set_title(sig["name"])
        ax.set_xlabel(f"Valor ({sig['unit']})")
        print(f"Cambiado a señal {current_signal}: {sig['name']}")


parser = argparse.ArgumentParser(description="Monitor de señales — SdC CDD")
parser.add_argument("--signal", type=int, choices=[0, 1], default=0,
                    help="Señal inicial (0 o 1)")
args = parser.parse_args()

current_signal = args.signal
select_signal(current_signal)
reset_buffers()

fig, ax = plt.subplots()
sig = SIGNALS[current_signal]
ax.set_title(sig["name"])
ax.set_xlabel(f"Valor ({sig['unit']})")
ax.set_ylabel("Tiempo (s)")
ax.set_xlim(0, 1800)
ax.set_ylim(0, 10)
line, = ax.plot([], [], "b-o", markersize=3)

fig.canvas.mpl_connect("key_press_event", on_key)

print(f"Mostrando {sig['name']}. Presioná 's' para cambiar de señal.")

ani = animation.FuncAnimation(fig, update, interval=1000, blit=False)
plt.tight_layout()
plt.show()
