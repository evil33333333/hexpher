#pragma once
#pragma once
#include <fstream>
#include <vector>
#include <windows.h>
#include "transmitter.h"

constexpr uintptr_t OUT_OF_BOUNDS = 0xFFFFFFFF;

struct f_content
{
	char* buffer = nullptr;
	size_t size = 0;
	bool ok = false;
};

struct pe_section
{
	std::string name;
	std::uintptr_t virtual_address;
	std::uintptr_t raw_address;
	std::uint64_t raw_size;
};


template <typename T>
bool contains_element(std::vector<T>* vec, T item)
{
	auto it = std::find(vec->begin(), vec->end(), item);
	bool result = it != vec->end();
	return result;
}


std::vector<pe_section> find_pe_sections(
	char* buffer
);

std::streampos return_file_size(
	std::ifstream* file
);

f_content get_file_content(
	const std::string& filename
);


std::vector<std::string> get_raw_bytes(
	std::string& byte_string
);

bool is_register(
	std::string& value
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

bool string_contains(
	std::string& haystack,
	const std::string& needle
);