import websocket
import json
import threading
import time
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime

# ── Configuration ────────────────────────────────────────────────────────────
WS_URL        = "ws://localhost:8080/carData"
MAX_POINTS    = 100          # rolling window of data points to show
UPDATE_MS     = 500          # graph refresh interval in milliseconds
FIELDS        = ["speed", "rpms", "engineTemp", "fuel"]  # keys to plot

# ── Shared state (written by WS thread, read by plot thread) ─────────────────
timestamps = deque(maxlen=MAX_POINTS)
buffers    = {field: deque(maxlen=MAX_POINTS) for field in FIELDS}
latest     = {}              # most-recent full packet for annotations
lock       = threading.Lock()

# ── WebSocket callbacks ───────────────────────────────────────────────────────
def on_message(ws_app, message):
    try:
        data = json.loads(message)
    except json.JSONDecodeError:
        return

    with lock:
        timestamps.append(datetime.now())
        for field in FIELDS:
            buffers[field].append(data.get(field, 0))
        latest.update(data)

def on_error(ws_app, error):
    print(f"[WS error] {error}")

def on_close(ws_app, close_status_code, close_msg):
    print("[WS] Connection closed.")

def on_open(ws_app):
    print("[WS] Connected.")

def start_websocket():
    ws_app = websocket.WebSocketApp(
        WS_URL,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
    )
    ws_app.run_forever()

# ── Plot setup ────────────────────────────────────────────────────────────────
COLOURS = ["#00C8FF", "#FF6B35", "#44D62C", "#FFD700"]

fig, axes = plt.subplots(len(FIELDS), 1, figsize=(12, 8), sharex=True)
fig.patch.set_facecolor("#0D0D0D")
fig.suptitle("Live Car Telemetry", color="white", fontsize=14, fontweight="bold", y=1.01)

lines = []
for ax, field, colour in zip(axes, FIELDS, COLOURS):
    ax.set_facecolor("#1A1A1A")
    ax.set_ylabel(field.replace("_", " ").title(), color=colour, fontsize=9)
    ax.tick_params(colors="grey", labelsize=7)
    for spine in ax.spines.values():
        spine.set_edgecolor("#333333")
    (line,) = ax.plot([], [], color=colour, linewidth=1.5, antialiased=True)
    lines.append(line)

axes[-1].set_xlabel("Time", color="grey", fontsize=8)
plt.tight_layout()

# ── Animation callback ────────────────────────────────────────────────────────
def update(_frame):
    with lock:
        if len(timestamps) < 2:
            return lines
        xs     = list(timestamps)
        data   = {f: list(buffers[f]) for f in FIELDS}
        snap   = dict(latest)

    for ax, line, field in zip(axes, lines, FIELDS):
        ys = data[field]
        line.set_data(xs, ys)
        ax.relim()
        ax.autoscale_view()

        # live value label on the right edge
        if ys:
            for txt in ax.texts:
                txt.remove()
            ax.text(
                0.99, 0.85,
                f"{ys[-1]:.1f}",
                transform=ax.transAxes,
                color=line.get_color(),
                fontsize=9, fontweight="bold",
                ha="right", va="top",
            )

    # Rotate x-axis tick labels on the bottom subplot only
    for label in axes[-1].get_xticklabels():
        label.set_rotation(30)
        label.set_color("grey")

    # Status / error banner in the figure title
    status_str = "● RUNNING" if snap.get("status") else "● STOPPED"
    error_count = len(snap.get("errors", []))
    gear = snap.get("gear", "–")
    fig.suptitle(
        f"Live Car Telemetry    |    Gear {gear}    |    {status_str}"
        + (f"    |    ⚠ {error_count} error(s)" if error_count else ""),
        color="white", fontsize=12, fontweight="bold",
    )

    return lines

# ── Entry point ───────────────────────────────────────────────────────────────
if __name__ == "__main__":
    # Start WebSocket in a background daemon thread
    ws_thread = threading.Thread(target=start_websocket, daemon=True)
    ws_thread.start()

    print(f"Connecting to {WS_URL} …  (close the plot window to quit)")

    ani = animation.FuncAnimation(
        fig, update, interval=UPDATE_MS, blit=False, cache_frame_data=False
    )

    plt.show()