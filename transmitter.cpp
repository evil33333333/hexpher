#include "transmitter.h"
#include "helper.h"

std::string translate_function(Function* function)
{
	uint32_t start_point = 0x04;
	std::string psuedo_code = std::format("func {}()\n", function->function_name);
	psuedo_code += "{\n";


	s_result println_res = search_for_byte_pattern(&function->instructions, "89 ?? ?? 89 ?? ?? ?? 8d ?? ?? ?? 89 ?? ?? ?? c7 ?? ?? ?? 01 00 00 00 c7 ?? ?? ?? 01 00 00 00 e8");
	if (println_res.found)
	{
		psuedo_code += std::format("\tfmt.Println([unknown_ptr (bad_static_ptr)])\n");
		start_point = println_res.index_found + 1;
	}
	

	// Skips the runtime.MoreStackNoCtxt jmp and the sub, rsp [0x??]
	for (size_t i = start_point; i < function->instructions.size(); i++)
	{

		if (function->instructions[i].second.find("call") != std::string::npos)
		{
			std::string func_addr = split_string(function->instructions[i].second, " ")[1];
			psuedo_code += std::format("\tgofunc_{}()\n", func_addr);
		}
		//psuedo_code += std::format("\t{}\n", function->instructions[i].second);
		
	}
	psuedo_code += "\n}";
	return psuedo_code;
}

std::vector<std::string> get_function_names(char* buffer, size_t buff_size)
{
	int null_count = 0;
	std::vector<unsigned char> _buffer;
	std::vector<std::string> function_names;
	populate_buffer(&_buffer, buffer, buff_size);
	std::string first_gofunc = "__x86.get_pc_thunk.ax";

	// Where the gofunc label table starts
	std::vector<unsigned char>::iterator start = std::search(_buffer.begin(), _buffer.end(), first_gofunc.begin(), first_gofunc.end());

	std::string function_name;
	for (;;)
	{
		if (null_count == 1 && *start == 0x00)
		{
			break;
		}
		else if (*start == 0x00)
		{
			function_names.push_back(function_name);
			function_name.clear();
			null_count++;
		}
		else
		{
			function_name += *start;
			null_count = 0;
		}
		++start;
	}

	/*
	
	function_names.erase(std::remove_if(function_names.begin(), function_names.end(),
		[](const std::string& x)
		{
			return x.find(".") == std::string::npos;
		}
	), function_names.end());
	
	*/
	

	function_names.erase(std::remove_if(function_names.begin(), function_names.end(),
		[](const std::string& x)
		{
			return x.find("func") != std::string::npos;
		}
	), function_names.end());

	return function_names;

}

std::vector<std::string> get_all_user_defined_functions(const std::vector<std::string>& total_funcs)
{
	std::vector<std::string> user_defined_functions;

	for (auto const& func : total_funcs)
	{
		if (func.find("main") != std::string::npos && func.find("runtime") == std::string::npos)
		{
			user_defined_functions.push_back(split_string(func, ".")[1]);
		}
	}

	return user_defined_functions;
}


std::vector<std::string> get_all_imports(const std::vector<std::string>& func_names)
{
	std::vector<std::string> imports;
	std::vector<std::string>::iterator result;
	for (auto const& func_name : func_names)
	{
		if (func_name.find("github.com") != std::string::npos)
		{
			std::string parsed_import = "github.com/";
			parsed_import += split_string(split_string(func_name, "github.com/")[1], ".")[0];
			result = std::find(imports.begin(), imports.end(), parsed_import);
			if (result == imports.end())
			{
				imports.push_back(parsed_import);
			}
		}

		else if (func_name.find(".") != std::string::npos)
		{
			std::string parsed_import = split_string(func_name, ".")[0];
			result = std::find(imports.begin(), imports.end(), parsed_import);
			if (result == imports.end() && parsed_import != "main" && parsed_import != "__x86")
			{
				imports.push_back(parsed_import);
			}
		}
	}
	return imports;

}

std::vector<Function> get_all_go_functions(char* buffer, size_t buff_size)
{
	std::vector<Function> functions;
	std::vector<unsigned char> _buffer;
	populate_buffer(&_buffer, buffer, buff_size);
	std::uintptr_t b_addr = 0;
	std::uintptr_t ret_addr = 0;

	std::vector<unsigned char>::iterator iter = _buffer.begin();
	for (;;)
	{	
		b_addr = find_next_function(ret_addr, &_buffer);

		char instructions[0xFF];
		char disassembled[0xFF];

		std::vector<std::pair<std::string, std::string>> code;
		for (std::uintptr_t i = b_addr, count = 0; i < buff_size; i += count)
		{
			count = disassemble((unsigned char*)buffer + i, buff_size - i, i, disassembled);
			instructions[0] = 0;
			for (int e = 0; e < count; e++)
			{
				sprintf(instructions + strlen(instructions), "%02x ", buffer[i + e]);
			}
			code.push_back({ instructions, disassembled });

			// Wait for the return
			if (code[code.size() - 1].second.find("ret") != std::string::npos)
			{
				ret_addr = i;
				break;
			}
		}
		
		if (b_addr == 0xFFFFFFFF)
		{
			break;
		}
		functions.push_back({ b_addr, std::format("gofunc_{}", b_addr), code });
	}

	return functions;
}