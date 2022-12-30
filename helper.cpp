#include "helper.h"
#include "transmitter.h"

std::streampos return_file_size(std::ifstream *file)
{
	file->seekg(0, std::ios::end);
	std::streampos length = file->tellg();
	file->seekg(0, std::ios::beg);
	return length;
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


std::string find_last_function(std::vector<std::string>* names)
{
	std::vector<std::string> reversed_names = *names;
	std::reverse(reversed_names.begin(), reversed_names.end());
	for (auto const& name : reversed_names)
	{
		if (name.rfind("type..eq.", 0) != std::string::npos || name.rfind("main.", 0) != std::string::npos)
		{
			return name;
		}
	}
	return std::string{};
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

std::vector<std::string> get_raw_bytes(std::string& byte_string)
{
	while (string_contains(byte_string, "ffffff"))
	{
		byte_string.replace(byte_string.find("ffffff"), 6, "");
	}
	std::vector<std::string> raw_bytes = split_string(byte_string, " ");
	raw_bytes.erase(std::remove_if(raw_bytes.begin(), raw_bytes.end(),
		[](const std::string& x)
	{
		return x == "";
	}
	), raw_bytes.end());
	return raw_bytes;

}

// replacement for value[0] != e, checks if value is a register or not
bool is_register(std::string& value)
{
	return value[0] == 0x65;
}


bool string_contains(std::string& haystack, const std::string& needle)
{
	return haystack.find(needle) != std::string::npos;
}

f_content get_file_content(const std::string& filename)
{
	f_content fc;
	std::memset((void*)&fc, 0x00, sizeof(f_content));

	FILE* file = fopen(filename.c_str(), "rb");
	if (!file)
	{
		return fc;
	}

	fseek(file, 0L, SEEK_END);
	long numbytes = ftell(file);
	fseek(file, 0L, SEEK_SET);
	char* bytes = (char*)calloc(numbytes, sizeof(char));

	if (!bytes)
	{
		return fc;
	}

	fread(bytes, sizeof(char), numbytes, file);

	fc.buffer = bytes;
	fc.size = numbytes;
	fc.ok;

	fclose(file);

	return fc;
}


void hexdump(FILE* file, int position, unsigned char* buffer)
{
	memset(buffer, 0, 0x500);
	fseek(file, position, SEEK_SET);
	(void)fread_s(buffer, 0x500, 1, 0x400, file);

}

PIMAGE_NT_HEADERS64 get_pimage_nt_headers(unsigned char* buffer, char* filename)
{

	FILE* file = fopen(filename, "rb");
	hexdump(file, 0, buffer);

	std::vector<pe_section> sections;
	PIMAGE_DOS_HEADER dhead = (PIMAGE_DOS_HEADER)buffer;
	hexdump(file, dhead->e_lfanew, buffer);

	fclose(file);

	PIMAGE_NT_HEADERS64 nt_headers = (PIMAGE_NT_HEADERS64)buffer;
	return nt_headers;

}

std::vector<pe_section> find_pe_sections(char* filename)
{
	std::vector<pe_section> sections;
	unsigned char buffer[0x500] = { 0 };
	PIMAGE_NT_HEADERS64 nt_headers = get_pimage_nt_headers(buffer, filename);
	PIMAGE_SECTION_HEADER pimage_sh = IMAGE_FIRST_SECTION(nt_headers);

	for (WORD i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
	{
		std::string name(reinterpret_cast<char*>(pimage_sh->Name), 8);

		pe_section pe_section{
			name,
			pimage_sh->VirtualAddress,
			pimage_sh->PointerToRawData,
			pimage_sh->SizeOfRawData,
		};
		sections.push_back(pe_section);

		pimage_sh++;
	}

	return sections;
}

std::string string_to_hex(const std::string& input)
{
	static const char hex_digits[] = "0123456789ABCDEF";

	std::string output;
	output.reserve(input.length() * 2);
	for (unsigned char c : input)
	{
		output.push_back(hex_digits[c >> 4]);
		output.push_back(hex_digits[c & 15]);
	}
	return output;
}

std::string duplicate_string(std::string text, uint32_t times)
{
	std::string result;
	for (uint32_t i = 0; i < times; i++)
	{
		result += text;
	}
	return result;
}

