#!/usr/bin/env python3
import glob
import hashlib
import os
import shutil
import subprocess
import sys

def main():
    frames_dir = sys.argv[1] if len(sys.argv) > 1 else "anim_frames"
    dist_dir = sys.argv[2] if len(sys.argv) > 2 else "dist"
    os.makedirs(dist_dir, exist_ok=True)

    gif_path = os.path.join(dist_dir, "tech-dictionary-v2-animation.gif")
    mp4_path = os.path.join(dist_dir, "tech-dictionary-v2-animation.mp4")

    pattern = os.path.join(frames_dir, "frame_%03d.png")
    frame_files = sorted(glob.glob(os.path.join(frames_dir, "frame_*.png")))

    print(f"Found {len(frame_files)} animation frames in {frames_dir}")
    if not frame_files:
        print("No animation frames found.")
        return 1
    frame_hashes = set()
    for frame in frame_files:
        with open(frame, "rb") as source:
            frame_hashes.add(hashlib.sha256(source.read()).digest())
    if len(frame_hashes) < 3:
        print("Animation frames do not contain enough visible changes.")
        return 1

    ffmpeg_bin = shutil.which("ffmpeg")
    ffmpeg_success = False

    if ffmpeg_bin:
        print(f"Using ffmpeg at {ffmpeg_bin} to assemble GIF and MP4...")
        try:
            # Generate high quality GIF with palettegen
            cmd_gif = [
                ffmpeg_bin, "-y", "-framerate", "12", "-i", pattern,
                "-vf", "fps=12,scale=1120:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse",
                gif_path
            ]
            subprocess.run(cmd_gif, check=True)
            print(f"Generated {gif_path} (size: {os.path.getsize(gif_path)} bytes)")

            # Generate MP4
            cmd_mp4 = [
                ffmpeg_bin, "-y", "-framerate", "12", "-i", pattern,
                "-c:v", "libx264", "-pix_fmt", "yuv420p",
                mp4_path
            ]
            subprocess.run(cmd_mp4, check=True)
            print(f"Generated {mp4_path} (size: {os.path.getsize(mp4_path)} bytes)")
            ffmpeg_success = True
        except Exception as e:
            print(f"ffmpeg failed: {e}")

    if not ffmpeg_success or not os.path.exists(gif_path):
        print("Falling back to Python Pillow for GIF generation...")
        try:
            from PIL import Image
            images = [Image.open(f) for f in frame_files]
            if images:
                images[0].save(
                    gif_path,
                    save_all=True,
                    append_images=images[1:],
                    duration=80,
                    loop=0
                )
                print(f"Generated {gif_path} using Pillow (size: {os.path.getsize(gif_path)} bytes)")
        except Exception as e:
            print(f"Pillow generation failed: {e}")

    if not os.path.isfile(gif_path) or os.path.getsize(gif_path) < 1000:
        print("Animation GIF is missing or empty.")
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
