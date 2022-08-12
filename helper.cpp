#include "helper.h"
#include "transmitter.h"

size_t return_file_size(std::ifstream *file) 
{
	file->seekg(0, std::ios::end);
	size_t length = file->tellg();
	file->seekg(0, std::ios::beg);
	return length;
}


void populate_buffer(std::vector<unsigned char>* new_buffer, char* old_buffer, size_t old_buffer_size)
{
	for (uint64_t i = 0; i < old_buffer_size; i++)
	{
		new_buffer->push_back(old_buffer[i]);
	}
}

std::uintptr_t find_next_function(std::uintptr_t prev_ret_addr, std::vector<unsigned char>* buffer)
{
	int gf_byteloc = 0;
	for (std::uintptr_t i = prev_ret_addr; i < buffer->size(); i++)
	{
		if (&gofunc_bytes[gf_byteloc] == std::end(gofunc_bytes))
		{
			return i;
		}
		else if (buffer->at(i) == gofunc_bytes[gf_byteloc])
		{
			gf_byteloc++;
		}
		else
		{
			gf_byteloc = 0;
		}
	}

	return -1;
}


std::vector<std::string> split_string(std::string content, std::string delimiter)
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = content.find(delimiter, pos_start)) != std::string::npos)
	{
		token = content.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(content.substr(pos_start));
	return res;
}

s_result search_for_byte_pattern(std::vector<std::pair<std::string, std::string>>* instructions, std::string byte_pattern)
{

	s_result result;
	int iter = 0;
	int kb_iter = 0;
	std::vector<std::string> key_bytes;
	std::vector<std::string> bytes = split_string(byte_pattern, " ");
	for (auto& instruction : *instructions)
	{
		for (;;)
		{
			if (instruction.first.find("ffffff") != std::string::npos)
			{
				instruction.first.replace(instruction.first.find("ffffff"), 6, "");
			}
			else
			{
				break;
			}
		}

		key_bytes = split_string(instruction.first, " ");
		

		// if we are currently on a return, return the result [end of func]
		if (instruction.second.find("ret") != std::string::npos)
		{
			return result;
		}

		for (auto const& kb : key_bytes)
		{
			if (iter == bytes.size() - 1)
			{
				result.found = true;
				result.index_found = find_index<std::pair<std::string, std::string>>(instructions, instruction);
				return result;
			}

			if (kb == "")
			{
				continue;
			}

			else if (bytes[iter] == "??" || bytes[iter] == kb)
			{
				iter++;
			}

			else if (bytes[iter] != kb)
			{
				iter = 0;
			}
		}
	}
	return result;
}


template <typename T> uint32_t find_index(std::vector<T>* vec, T item)
{
	auto it = std::find(vec->begin(), vec->end(), item);
	if (it == vec->end())
	{
		return -1;
	}
	else
	{
		return it - vec->begin();
	}
}