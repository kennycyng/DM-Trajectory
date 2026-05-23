#ifndef __Binary_Trajectory_Writer_hpp_
#define __Binary_Trajectory_Writer_hpp_

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace DaMaSCUS_SUN
{

// Magic number: "DMST" in little-endian ASCII
constexpr uint64_t BINARY_TRAJECTORY_MAGIC = 0x44534D54;  // "DMST"
constexpr uint64_t BINARY_TRAJECTORY_VERSION = 1;

#pragma pack(push, 1)
struct TrajectoryPoint
{
	double t;
	double x;
	double y;
	double z;
	double vx;
	double vy;
	double vz;
	double E;
};

struct TrajectoryHeader
{
	uint64_t trajectory_id;
	uint64_t n_points;
};

struct FileHeader
{
	uint64_t magic;
	uint64_t version;
	uint64_t num_trajectories;
};
#pragma pack(pop)

class BinaryTrajectoryWriter
{
  private:
	FILE* file = nullptr;
	std::vector<TrajectoryPoint> buffer;
	std::size_t buffer_idx = 0;

	uint64_t current_trajectory_id = 0;
	uint64_t current_trajectory_points = 0;
	long current_trajectory_header_pos = 0;
	bool trajectory_open = false;
	uint64_t total_trajectories = 0;

	std::string current_path;
	int mpi_rank = 0;
	int chunk_id = 0;
	std::string output_dir;

	void FlushBuffer();
	std::string MakePath(int chunk);

  public:
	explicit BinaryTrajectoryWriter(std::size_t buffer_points = 1024);
	~BinaryTrajectoryWriter();

	void Open(int rank, const std::string& dir);
	void BeginTrajectory(uint64_t trajectory_id);
	void WritePoint(const TrajectoryPoint& point);
	void EndTrajectory();
	void Close();

	bool IsOpen() const { return file != nullptr; }
	std::string CurrentPath() const { return current_path; }
};

}  // namespace DaMaSCUS_SUN

#endif
