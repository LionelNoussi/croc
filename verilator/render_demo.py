# render_video.py

import os
import sys
import numpy as np
import matplotlib.pyplot as plt # type: ignore
from matplotlib.animation import FuncAnimation, PillowWriter # type: ignore


WIDTH = 128
HEIGHT = 128         # Full frame height
HEIGHT_CHUNKS = 32   # Number of vertical chunks per frame
H_CHUNK_SIZE = HEIGHT // HEIGHT_CHUNKS

def extract_timestamp(filename):
    return int(filename.split('.')[0].split('_')[-1].replace('ns', ''))

def load_frames(directory):
    chunk_files = sorted(
        [f for f in os.listdir(directory) if f.endswith(".hex")],
        key=extract_timestamp
    )

    chunk_frames = []
    timestamps = []

    for filename in chunk_files:
        path = os.path.join(directory, filename)
        with open(path, "r") as f:
            data = [int(line.strip(), 16) for line in f]
            if len(data) != WIDTH * H_CHUNK_SIZE:
                print(f"Skipping {filename}: invalid chunk size")
                continue
            chunk = np.array(data, dtype=np.uint8).reshape((H_CHUNK_SIZE, WIDTH))
            chunk_frames.append(chunk)
            timestamps.append(extract_timestamp(filename))

    # Combine HEIGHT_CHUNKS into full frames
    full_frames = []
    full_timestamps = []
    for i in range(0, len(chunk_frames), HEIGHT_CHUNKS):
        if i + HEIGHT_CHUNKS > len(chunk_frames):
            break
        frame = np.vstack(chunk_frames[i:i + HEIGHT_CHUNKS])
        full_frames.append(frame)
        full_timestamps.append(timestamps[i])  # use timestamp of first chunk

    # Compute average delta in nanoseconds → convert to milliseconds
    deltas = [t2 - t1 for t1, t2 in zip(full_timestamps[:-1], full_timestamps[1:])]
    avg_ns = sum(deltas) / len(deltas) if deltas else 1e8
    avg_ms = avg_ns / 1e6  # ms for matplotlib interval
    return full_frames, avg_ms


def render(frames, interval_ms, filename):
    fig, ax = plt.subplots()
    im = ax.imshow(frames[0], cmap="gray", vmin=0, vmax=255)
    ax.axis("off")

    def update(i):
        im.set_array(frames[i])
        return [im]

    print(f"Interval: {interval_ms}")
    anim = FuncAnimation(fig, update, frames=len(frames), interval=interval_ms, blit=True)
    plt.show()
    # anim.save(filename + '.mp4', fps=1000//interval_ms, writer='ffmpeg') # needs ffmpeg installed
    # anim.save(filename + '.gif', writer=PillowWriter(fps=1000//interval_ms))


if __name__ == "__main__":
    frame_dir = "verilator/videoframes/final_demo/"
    out_filename = "demo_video"

    if len(frame_dir) == 0 or len(out_filename) == 0:
        print("Please specify the directory of the saved frames" +
              "or the output filename of the video.")
        exit()

    frames, interval_ms = load_frames(frame_dir)
    if not frames:
        print("No valid frames found.")
        sys.exit(1)

    print(f"Rendering at ~{interval_ms:.2f} ms per frame")
    render(frames, 67.24, out_filename)