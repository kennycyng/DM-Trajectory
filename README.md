# DaMaSCUS-SUN Trajectory


## Portable Package

This directory is intended to be copied as a source-only package. It should contain only the project source, vendored DaMaSCUS/obscura/libphysica source, configs, and documentation.

Do not copy generated or machine-local directories such as `.deps/`, `build/`, `build-*`, `output/`, or `smoke_output/`. Install third-party toolchains and libraries on the target machine, then configure a fresh `build/` directory there.

## Required Versions

- CMake 3.21.2
- Boost 1.77.0
- Intel oneAPI 2022.2 with Intel C++ 2022.1.0
- Intel MPI 2021.6.0
- A C++11 build mode
- libconfig++

The project intentionally pins Boost to 1.77.0 because newer Boost.Math releases require C++14 and break the C++11 DaMaSCUS/libphysica build.

## Install Dependencies On A New Machine

### Cluster Modules

If the target machine provides environment modules, prefer the site-managed modules and load the exact versions before configuring:

```bash
module purge
module load cmake/3.21.2
module load intel-oneapi/2022.2
module load intel-mpi/2021.6.0
module load boost/1.77.0
module load libconfig
```

### Manual CMake 3.21.2 Install

If CMake 3.21.2 is not provided as a module, install the official binary release outside this project directory:

```bash
mkdir -p $HOME/opt
cd $HOME/opt
curl -L -o cmake-3.21.2-linux-x86_64.tar.gz https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-linux-x86_64.tar.gz
tar -xzf cmake-3.21.2-linux-x86_64.tar.gz
export PATH=$HOME/opt/cmake-3.21.2-linux-x86_64/bin:$PATH
```

### Manual Boost 1.77.0 Install

If Boost 1.77.0 is not provided as a module, install it outside this project directory. This code only uses Boost headers, so extracting the official source archive is sufficient:

```bash
mkdir -p $HOME/opt
cd $HOME/opt
curl -L -o boost_1_77_0.tar.gz https://archives.boost.io/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xzf boost_1_77_0.tar.gz
export BOOST_ROOT=$HOME/opt/boost_1_77_0
```

### Intel oneAPI And Intel MPI

Intel oneAPI 2022.2 and Intel MPI 2021.6.0 are normally installed by the cluster administrator or loaded through modules. If they are manually installed under the default prefix, initialize them before configuring:

```bash
source /opt/intel/oneapi/setvars.sh
export I_MPI_CC=icx
export I_MPI_CXX=icpx
```

Use the Intel MPI compiler wrappers for this project. On many oneAPI 2022.2 installations these are named `mpiicc` and `mpiicpc`.

### libconfig++

Install libconfig++ through the target machine's package manager or module system:

```bash
# Ubuntu or Debian
sudo apt update
sudo apt install -y libconfig++-dev

# RHEL or Rocky Linux with EPEL enabled
sudo dnf install -y libconfig-devel
```

Check that the expected tools are active:

```bash
cmake --version
icpx --version
mpiicpc -show
mpirun --version
```

The Boost check should report version 1.77.0 during CMake configure. If it reports another version, remove the build directory and reconfigure with the `BOOST_ROOT` and `Boost_INCLUDE_DIR` options shown below.

## Build

Copy this source package to the target machine, install dependencies as above, then configure with Intel MPI compiler wrappers and the pinned Boost 1.77.0 include tree:

```bash
cd trajectory_txt_container
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

On the cluster sandbox, the full flow is:

```bash
ssh sandbox
cd /project/kennyng/backup_DM/trajectory_txt_container
module purge
module load cmake/3.21.2
module load intel-oneapi/2022.2
module load intel-mpi/2021.6.0
module load boost/1.77.0
module load libconfig
if [ -z "$BOOST_ROOT" ]; then
	export BOOST_ROOT=$HOME/opt/boost_1_77_0
fi
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

If the Boost module does not set `BOOST_ROOT`, set it manually to the Boost 1.77.0 prefix, for example:

```bash
export BOOST_ROOT=$HOME/opt/boost_1_77_0
```

## Run

Quick smoke test after building:

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
head -5 smoke_output/results_*/trajectory_1_task0.txt
```

Normal run:

```bash
mpirun -np 4 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

Inside a SLURM job, for example with 32 MPI processes:

```bash
mpirun -np 32 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

The output directory is controlled by `output_dir` in the config file. Trajectory file names look like:

```text
trajectory_1_task0.txt
trajectory_2_task0.txt
trajectory_1_task1.txt
```

Each `.txt` file has these columns:

```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

