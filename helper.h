#pragma once
#include <fstream>
#include <vector>
#include "transmitter.h"

struct s_result 
{
	bool found = false;
	int index_found = -1;
};

size_t return_file_size(std::ifstream* file);
void populate_buffer(std::vector<unsigned char>* new_buffer, char* old_buffer, size_t old_buffer_size);
std::uintptr_t find_next_function(std::uintptr_t prev_ret_addr, std::vector<unsigned char>* buffer);
std::vector<std::string> split_string(std::string content, std::string delimiter);
void label_all_functions(std::vector<Function>* functions, std::vector<std::string>* names);
s_result search_for_byte_pattern(std::vector<std::pair<std::string, std::string>>* instructions, std::string byte_pattern);
template <typename T> uint32_t find_index(std::vector<T>* vec, T item);