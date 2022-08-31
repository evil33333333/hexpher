#pragma once
#include <fstream>
#include <vector>
#include <windows.h>
#include "transmitter.h"

struct s_result 
{
	bool found = false;
	int index_found = -1;
};

class f_content
{
public:
	char* buffer;
	size_t size;
	bool ok = false;
};

struct pe_section
{
	std::string name;
	std::uintptr_t virtual_address;
	std::uintptr_t raw_address;
	std::uint64_t raw_size;
};

std::vector<pe_section> find_pe_sections(
	char* buffer
);

std::size_t return_file_size(
	std::ifstream* file
);

f_content get_file_content(
	const std::string& filename
);


std::vector<std::string> get_raw_bytes(
	std::string& byte_string
);

template <typename T> uint32_t find_index(
	std::vector<T>* vec,
	T item
);

std::vector<std::string> split_string(
	std::string content,
	std::string delimiter
);

void label_all_functions(
	std::vector<Function>* functions,
	std::vector<std::string>* names
);


std::uintptr_t find_next_function(
	std::uintptr_t prev_ret_addr,
	std::vector<unsigned char>* buffer
);

s_result search_for_byte_pattern(
	std::vector<std::pair<std::string, std::string>>* instructions,
	std::string byte_pattern
);

void populate_buffer(
	std::vector<unsigned char>* new_buffer,
	char* old_buffer,
	size_t old_buffer_size
);

std::string string_to_hex(
	const std::string& input
);

std::string duplicate_string(
	std::string text,
	uint32_t times
);

std::string find_last_function(
	std::vector<std::string>* names
);