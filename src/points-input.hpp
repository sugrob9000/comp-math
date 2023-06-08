#pragma once

#include <math.hpp>
#include <span>
#include <string_view>
#include <vector>

class Points_input {
	std::vector<dvec2> points;
	dvec2 new_point_input = {};

	enum class File_load_status { ok, unreadable, bad_data };
	File_load_status last_file_load_status = File_load_status::ok;
	constexpr static size_t path_buf_size = 256;
	char path_buf[path_buf_size];

public:
	Points_input () = default;
	Points_input (std::string_view initial_path);

	std::span<const dvec2> view () const { return points; }

	void try_load_file ();
	bool widget ();
};