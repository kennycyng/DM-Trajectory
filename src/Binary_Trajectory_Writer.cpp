#include "Binary_Trajectory_Writer.hpp"

#include <iostream>
#include <sstream>

namespace DaMaSCUS_SUN
{

BinaryTrajectoryWriter::BinaryTrajectoryWriter(std::size_t buffer_points)
	: buffer(buffer_points)
{
}

BinaryTrajectoryWriter::~BinaryTrajectoryWriter()
{
	Close();
}

std::string BinaryTrajectoryWriter::MakePath(int chunk)
{
	std::ostringstream oss;
	if(chunk == 0)
		oss << output_dir << "/trajectories_rank" << mpi_rank << ".bin";
	else
		oss << output_dir << "/trajectories_rank" << mpi_rank << "_chunk" << chunk << ".bin";
	return oss.str();
}

void BinaryTrajectoryWriter::Open(int rank, const std::string& dir)
{
	Close();
	mpi_rank = rank;
	output_dir = dir;
	chunk_id = 0;
	current_path = MakePath(chunk_id);
	file = std::fopen(current_path.c_str(), "wb");
	if(!file)
	{
		std::cerr << "Error: Cannot open binary trajectory file: " << current_path << std::endl;
		return;
	}

	FileHeader fh{BINARY_TRAJECTORY_MAGIC, BINARY_TRAJECTORY_VERSION, 0};
	std::fwrite(&fh, sizeof(FileHeader), 1, file);
	total_trajectories = 0;
	buffer_idx = 0;
}

void BinaryTrajectoryWriter::FlushBuffer()
{
	if(buffer_idx == 0 || !file)
		return;
	std::fwrite(buffer.data(), sizeof(TrajectoryPoint), buffer_idx, file);
	buffer_idx = 0;
}

void BinaryTrajectoryWriter::BeginTrajectory(uint64_t trajectory_id)
{
	if(!file)
		return;
	FlushBuffer();
	current_trajectory_id = trajectory_id;
	current_trajectory_points = 0;
	trajectory_open = true;

	// Remember where we will write the header so we can patch n_points later
	current_trajectory_header_pos = std::ftell(file);
	TrajectoryHeader th{trajectory_id, 0};
	std::fwrite(&th, sizeof(TrajectoryHeader), 1, file);
}

void BinaryTrajectoryWriter::WritePoint(const TrajectoryPoint& point)
{
	if(!file || !trajectory_open)
		return;
	buffer[buffer_idx++] = point;
	current_trajectory_points++;
	if(buffer_idx >= buffer.size())
		FlushBuffer();
}

void BinaryTrajectoryWriter::EndTrajectory()
{
	if(!file || !trajectory_open)
		return;
	FlushBuffer();

	// Seek back and patch the n_points field
	long current_pos = std::ftell(file);
	std::fseek(file, current_trajectory_header_pos, SEEK_SET);
	TrajectoryHeader th{current_trajectory_id, current_trajectory_points};
	std::fwrite(&th, sizeof(TrajectoryHeader), 1, file);
	std::fseek(file, current_pos, SEEK_SET);

	trajectory_open = false;
	total_trajectories++;
}

void BinaryTrajectoryWriter::Close()
{
	if(!file)
		return;
	if(trajectory_open)
		EndTrajectory();
	FlushBuffer();

	// Patch total trajectory count in file header
	std::fseek(file, 0, SEEK_SET);
	FileHeader fh{BINARY_TRAJECTORY_MAGIC, BINARY_TRAJECTORY_VERSION, total_trajectories};
	std::fwrite(&fh, sizeof(FileHeader), 1, file);

	std::fclose(file);
	file = nullptr;
}

}  // namespace DaMaSCUS_SUN
