#!/usr/bin/env python3
"""
Read DaMaSCUS-SUN binary trajectory files.

Usage:
    python read_binary_trajectories.py path/to/trajectories_rank0.bin
"""

import sys
import struct
import numpy as np

# Struct formats (little-endian)
FILE_HEADER_FMT = '<QQQ'      # magic, version, num_trajectories
TRAJ_HEADER_FMT = '<QQ'       # trajectory_id, n_points
POINT_FMT = '<8d'             # t, x, y, z, vx, vy, vz, E

FILE_HEADER_SIZE = struct.calcsize(FILE_HEADER_FMT)
TRAJ_HEADER_SIZE = struct.calcsize(TRAJ_HEADER_FMT)
POINT_SIZE = struct.calcsize(POINT_FMT)

MAGIC = 0x44534D54  # "DMST"


def read_binary_trajectories(filepath):
    """Read a binary trajectory file and return list of (id, points_array)."""
    trajectories = []
    with open(filepath, 'rb') as f:
        magic, version, num_trajectories = struct.unpack(FILE_HEADER_FMT, f.read(FILE_HEADER_SIZE))
        if magic != MAGIC:
            raise ValueError(f"Bad magic: expected {MAGIC:#x}, got {magic:#x}")
        if version != 1:
            raise ValueError(f"Unknown version: {version}")

        for _ in range(num_trajectories):
            traj_id, n_points = struct.unpack(TRAJ_HEADER_FMT, f.read(TRAJ_HEADER_SIZE))
            raw = f.read(n_points * POINT_SIZE)
            points = np.frombuffer(raw, dtype=np.dtype([
                ('t', np.float64), ('x', np.float64), ('y', np.float64), ('z', np.float64),
                ('vx', np.float64), ('vy', np.float64), ('vz', np.float64), ('E', np.float64)
            ]))
            trajectories.append((traj_id, points))

    return trajectories


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <trajectories_rankN.bin>")
        sys.exit(1)

    filepath = sys.argv[1]
    trajectories = read_binary_trajectories(filepath)

    print(f"File: {filepath}")
    print(f"Trajectories: {len(trajectories)}")
    total_points = sum(len(p) for _, p in trajectories)
    print(f"Total points: {total_points}")
    print()

    # Show first trajectory
    if trajectories:
        tid, pts = trajectories[0]
        print(f"First trajectory (id={tid}):")
        print(f"  Points: {len(pts)}")
        print(f"  First point: t={pts['t'][0]:.4e}, x={pts['x'][0]:.4e}, E={pts['E'][0]:.4e}")
        print(f"  Last point:  t={pts['t'][-1]:.4e}, x={pts['x'][-1]:.4e}, E={pts['E'][-1]:.4e}")


if __name__ == '__main__':
    main()
