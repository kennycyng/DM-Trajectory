# DaMaSCUS-SUN Trajectory

Trajectory-only runner for DaMaSCUS-SUN. The executable simulates dark-matter trajectories through the Sun and writes them to output files.

## What's new in this fork

- **Binary output mode** (`output_format = "binary"`) — ~1.7× faster I/O, ~3.5× smaller files, and no more 100,000+ text files cluttering the filesystem.
- **Rate interpolation cache** — subsequent runs with the same DM mass but different cross sections skip the expensive 2D table build by loading a cached binary file.
- **Python reader** — `scripts/read_binary_trajectories.py` loads binary trajectory files into NumPy arrays.

---

## Requirements

- CMake 3.21.2+
- Boost 1.77.0 (exact version — newer Boost.Math requires C++14)
- MPI (Open MPI or Intel MPI)
- A C++11 build mode
- libconfig++

### macOS (Homebrew)

```bash
brew install cmake libconfig open-mpi
```

Install Boost 1.77.0 as a header prefix:

```bash
mkdir -p $HOME/opt && cd $HOME/opt
curl -L -o boost_1_77_0.tar.gz https://archives.boost.io/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xzf boost_1_77_0.tar.gz
export BOOST_ROOT=$HOME/opt/boost_1_77_0
```

### Linux

```bash
# Ubuntu/Debian
sudo apt install -y libconfig++-dev cmake mpi-default-dev

# Or with environment modules
module load cmake/3.21.2 boost/1.77.0 libconfig
```

---

## Build

### macOS with Homebrew MPI

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DBoost_NO_BOOST_CMAKE=ON \
    -DBoost_NO_SYSTEM_PATHS=ON \
    -DBOOST_ROOT=$BOOST_ROOT \
    -DBoost_INCLUDE_DIR=$BOOST_ROOT
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
```

### Linux with Intel MPI

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=mpiicc \
    -DCMAKE_CXX_COMPILER=mpiicpc \
    -DBoost_NO_BOOST_CMAKE=ON \
    -DBoost_NO_SYSTEM_PATHS=ON \
    -DBOOST_ROOT=$BOOST_ROOT \
    -DBoost_INCLUDE_DIR=$BOOST_ROOT
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
```

---

## Run

### Smoke test (text output — default)

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
head -5 smoke_output/results_*/trajectory_1_task0.txt
```

### Smoke test (binary output)

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke_binary.cfg
python3 scripts/read_binary_trajectories.py \
    smoke_binary_output/results_*/trajectories_rank0.bin
```

### Normal run

```bash
mpirun -np 4 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

---

## Output formats

The output format is controlled by `output_format` in the config file.

### `output_format = "txt"` (default)

Creates one text file per trajectory:
```
trajectory_1_task0.txt
trajectory_2_task0.txt
trajectory_1_task1.txt
...
```

Each `.txt` file has these columns:
```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

### `output_format = "binary"`

Creates one binary file per MPI rank:
```
trajectories_rank0.bin
trajectories_rank1.bin
...
```

**Format specification:**
```
[FileHeader]   magic="DMST", version=1, num_trajectories
  [TrajectoryHeader]   trajectory_id, n_points
    [TrajectoryPoint × n_points]   t, x, y, z, vx, vy, vz, E  (8 doubles = 64 bytes)
  [TrajectoryHeader] ...
```

**Why use binary?**
- **~1.7× faster** wall time (less CPU spent formatting text, fewer filesystem syscalls)
- **~3.5× smaller** files (64 bytes/point vs ~140 bytes/point in text)
- **No 100k+ file explosion** — one file per rank instead of one per trajectory

### Reading binary output in Python

```python
from scripts.read_binary_trajectories import read_binary_trajectories

trajectories = read_binary_trajectories("output/results_*/trajectories_rank0.bin")
for traj_id, points in trajectories:
    print(f"Trajectory {traj_id}: {len(points)} points")
    print(points['t'])   # array of time values
    print(points['E'])   # array of energy values
```

---

## Configuration file reference

Key settings in the `.cfg` file:

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `output_dir` | string | `"./output"` | Parent output directory |
| `output_format` | string | `"txt"` | `"txt"` or `"binary"` |
| `binary_buffer_size` | int | `1024` | Points to buffer before disk write (binary mode only) |
| `sample_size` | int | `1000` | Target number of captured trajectories |
| `max_trajectories` | int | `0` | Hard limit on total trajectories (0 = unlimited) |
| `trajectory_write_stride` | int | `1` | Write 1 row every N accepted RK45 steps |
| `txt_precision` | int | `10` | Decimal digits for text output |
| `interpolation_points` | int | `1000` | Grid resolution for 2D rate interpolation |
| `initial_radius_rsun` | float | `2.0` | Outer boundary in solar radii |
| `maximum_number_of_scatterings` | int | huge | Scatterings allowed per trajectory |
| `maximum_free_time_steps` | int | huge | RK45 steps allowed per trajectory |
| `max_trajectory_wall_time_sec` | float | `300.0` | Kill switch for stuck trajectories |
| `capture_mode` | bool | `false` | Stop when E < 0 (approximate capture) |
| `clear_existing_trajectories` | bool | `true` | Delete old `trajectory_*` files before run |

### DM particle settings

| Setting | Description |
|---------|-------------|
| `DM_mass` | Mass in GeV |
| `DM_spin` | Spin in ℏ |
| `DM_light` | `true` for low-mass optimizations |
| `DM_interaction` | `"SI"`, `"SD"`, or `"DP"` |
| `DM_cross_section_nucleon` | Nucleon cross section in cm² |
| `DM_cross_section_electron` | Electron cross section in cm² |
| `DM_form_factor` | `"Contact"`, `"General"`, `"Long-Range"`, `"Electric-Dipole"` |
| `DM_mediator_mass` | Mediator mass in MeV (only for `"General"`) |

### Halo model settings

| Setting | Description |
|---------|-------------|
| `DM_distribution` | `"SHM"`, `"SHM++"`, or `"File"` |
| `DM_local_density` | Local DM density in GeV/cm³ |
| `SHM_v0` | Velocity dispersion in km/s |
| `SHM_vObserver` | Observer velocity vector `(vx, vy, vz)` in km/s |
| `SHM_vEscape` | Galactic escape velocity in km/s |

---

## Rate interpolation cache

On the first run for a given DM mass, the code builds a 2D interpolation table of scattering rates and saves it to:
```
<output_dir>/.cache/rate_<interaction>_m<mass>_lm<low_mass>_sp<sigma_p>_se<sigma_e>.bin
```

Subsequent runs with the **same mass and interaction type** (but different cross sections) will load this cache instead of rebuilding the table, saving ~0.5–5 seconds depending on core count.

To force a rebuild, simply delete the `.cache/` directory.

---

## Performance tips

| Bottleneck | Fix |
|-----------|-----|
| Slow I/O with many trajectories | Use `output_format = "binary"` |
| Huge text files | Increase `trajectory_write_stride` (e.g., `100`) |
| Startup delay in parameter scans | Reuse the same DM mass so the cache hits |
| Stuck trajectories | Lower `max_trajectory_wall_time_sec` |

---

## File layout

```
.
├── CMakeLists.txt
├── README.md
├── config/
│   ├── example.cfg          # Standard example
│   ├── smoke.cfg            # Minimal smoke test (text)
│   └── smoke_binary.cfg     # Minimal smoke test (binary)
├── scripts/
│   └── read_binary_trajectories.py
├── src/
│   ├── main.cpp
│   ├── Binary_Trajectory_Writer.hpp
│   └── Binary_Trajectory_Writer.cpp
└── vendor/
    └── damascus/
        ├── data/            # AGSS09 solar model
        ├── include/         # Headers (Solar_Model, Simulation_Trajectory, ...)
        └── src/             # Source files
```
