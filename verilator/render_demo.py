# render_video.py

import os
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import re

WIDTH = 16
HEIGHT = 8

def extract_timestamp(fname):
    match = re.search(r"_(\d+)ns\.hex$", fname)
    return int(match.group(1)) if match else 0

def load_frames(directory):
    files = [f for f in os.listdir(directory) if f.endswith(".hex")]
    files.sort(key=extract_timestamp)

    frames = []
    for fname in files:
        with open(os.path.join(directory, fname), "r") as f:
            data = [int(line.strip(), 16) for line in f]
            frame = np.array(data, dtype=np.uint8).reshape((HEIGHT, WIDTH))
            frame = np.flipud(frame)  # flip vertically to match C rendering
            frames.append(frame)
    return frames

def render_animation(frames):
    fig, ax = plt.subplots()
    im = ax.imshow(frames[0], cmap='gray', vmin=0, vmax=1)

    def update(i):
        im.set_array(frames[i])
        return [im]

    ani = FuncAnimation(fig, update, frames=len(frames), interval=100, blit=True, repeat=False)
    plt.axis('off')
    plt.show()

if __name__ == "__main__":
    frame_dir = "verilator/videoframes/"
    frames = load_frames(frame_dir)
    render_animation(frames)
