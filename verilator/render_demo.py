# render_video.py

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

WIDTH = 32
HEIGHT = 16

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

WIDTH = 64
HEIGHT = 16

def load_frames(directory):
    frames = []
    for filename in sorted(os.listdir(directory), key=lambda a: int(a.split('.')[0].split('_')[-1].replace('ns', ''))):
        if not filename.endswith(".hex"):
            continue
        path = os.path.join(directory, filename)
        with open(path, "r") as f:
            data = [int(line.strip(), 16) for line in f]
            if len(data) != WIDTH * HEIGHT:
                print(f"Skipping {filename}: invalid size")
                continue
            frame = np.array(data, dtype=np.uint8).reshape((HEIGHT, WIDTH))
            frames.append(frame)
    return frames

def render(frames):
    fig, ax = plt.subplots()
    im = ax.imshow(frames[0], cmap="gray", vmin=0, vmax=255)
    ax.axis("off")

    def update(i):
        im.set_array(frames[i])
        return [im]

    anim = FuncAnimation(fig, update, frames=len(frames), interval=100, blit=True)
    plt.show()


if __name__ == "__main__":
    frame_dir = "verilator/videoframes/"
    frames = load_frames(frame_dir)
    if not frames:
        print("No valid frames found.")
        sys.exit(1)

    render(frames)
