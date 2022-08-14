#include "transmitter.h"
#include "helper.h"

std::string translate_function(Function* function, char* buffer)
{
	uint32_t start_point = 0x04;
	uint32_t string_count = 0x00;
	uint32_t int_count = 0x00;
	uint32_t boolean_count = 0x00;

	std::queue<std::string> stack;
	std::vector<uint32_t> instructions_used;
	std::map<std::string, std::uintptr_t> integer_variables;
	std::string psuedo_code = std::format("func {}()\n", function->function_name);
	psuedo_code += "{\n";

	// Skips the runtime.MoreStackNoCtxt jmp and the sub, rsp [0x??]
	for (size_t i = start_point; i < function->instructions.size(); i++)
	{

		if (std::find(instructions_used.begin(), instructions_used.end(), i) == instructions_used.end())
		{
			if (function->instructions[i].second.find("call") != std::string::npos)
			{

				instructions_used.push_back(i);
				std::string func_addr = split_string(function->instructions[i].second, " ")[1];

				if (function->instructions[i + 1].second.find("mov e") != std::string::npos && function->instructions[i - 2].second.find("lea") == std::string::npos)
				{
					instructions_used.push_back(i + 1);
					psuedo_code += std::format("\tdata, err := gofunc_{}(", func_addr);
				}
				else
				{
					psuedo_code += std::format("\terr := gofunc_{}(", func_addr);
				}
				while (!stack.empty())
				{
					psuedo_code += stack.size() == 1 ? std::format("{}", stack.front()) : std::format("{}, ", stack.front());;
					stack.pop();
				}
				psuedo_code += ")\n";
			}
			//psuedo_code += std::format("\t{}\n", function->instructions[i].second);

			if (function->instructions[i].second.find("mov BYTE PTR") != std::string::npos)
			{
				instructions_used.push_back(i);

				int value;
				std::stringstream stream;
				std::string byte_string = split_string(function->instructions[i].second, ",")[1];

				stream << std::hex << byte_string;
				stream >> value;


				psuedo_code += std::format("\tvar boolean_{} bool = {}\n", boolean_count, value == 1 ? "true" : "false");
				stack.push(std::format("boolean_{}", boolean_count));
				boolean_count++;
			}

			if (function->instructions[i].second.find("mov DWORD PTR") != std::string::npos && function->instructions[i - 1].second.find("lea") == std::string::npos)
			{
				signed int value;
				std::uintptr_t address;
				std::stringstream stream;
				instructions_used.push_back(i);
				std::string integer = split_string(function->instructions[i].second, ",")[1];

				if (integer[0] == 'e')
				{
					psuedo_code += std::format("\tvar integer_{} int\n", int_count);
					std::string int_addr = split_string(split_string(function->instructions[i].second, "[")[1], "]")[0];

					// if we are doing, mov from variable to register, skip
					if (int_addr[0] == 'e')
					{
						continue;
					}
					
					stream << std::hex << int_addr;
					stream >> address;

					integer_variables[std::format("integer_{}", int_count)] = address;
					stack.push(std::format("integer_{}", int_count));

				}
				else
				{
					stream << std::hex << integer;
					stream >> value;
					psuedo_code += std::format("\tinteger_{} := {}\n", int_count, value);
					stack.push(std::format("integer_{}", int_count));
					stream.clear();
				}
				int_count++;
			}


			if (function->instructions[i].second.find("test") != std::string::npos && function->instructions[i + 1].second.find("jz") != std::string::npos)
			{
				instructions_used.push_back(i);
				instructions_used.push_back(i + 1);
				psuedo_code += "\tif err != nil {  \n\t\tpanic(err)\n  \t}\n";
			}



			if (function->instructions[i].second.find("lea") != std::string::npos && function->instructions[i + 2].second.find("mov") != std::string::npos)
			{
				int virtual_address = 0;
				int string_size = 0;

				std::string string;
				std::stringstream stream;

				instructions_used.push_back(i);
				

				std::string string_addr = split_string(split_string(function->instructions[i].second, "[")[1], "]")[0];
				stream << std::hex << string_addr;
				stream >> virtual_address;

				stream.clear();

				std::string str_size = split_string(function->instructions[i + 2].second, ",")[1];

				if (str_size[0] != 'e')
				{
					instructions_used.push_back(i + 1);
					instructions_used.push_back(i + 2);
					stream << std::hex << str_size;
					stream >> string_size;

					virtual_address = virtual_address - 0x00401000;


					for (int ptr = virtual_address; ptr < virtual_address + string_size; ptr++)
					{
						string += buffer[ptr];
					}
					psuedo_code += std::format("\tstring_{} := \"{}\"\n", string_count, string);
					stack.push(std::format("string_{}", string_count));
					string_count++;
				}
				
			}
		}
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


	/*function_names.erase(std::remove_if(function_names.begin(), function_names.end(),
		[](const std::string& x)
		{
			return x.find("func") != std::string::npos;
		}
	), function_names.end());*/

	return function_names;

}

std::vector<std::string> get_strings_from_function(Function* function, char* buffer)
{
	std::vector<std::string> strings;
	for (size_t i = 0x04; i < function->instructions.size(); i++)
	{

		if (function->instructions[i].second.find("lea") != std::string::npos && function->instructions[i + 2].second.find("mov") != std::string::npos)
		{
			int virtual_address = 0;
			int string_size = 0;

			std::string string;
			std::stringstream stream;

			std::string string_addr = split_string(split_string(function->instructions[i].second, "[")[1], "]")[0];
			stream << std::hex << string_addr;
			stream >> virtual_address;

			stream.clear();

			std::string str_size = split_string(function->instructions[i + 2].second, ",")[1];
			stream << std::hex << str_size;
			stream >> string_size;

			virtual_address = virtual_address - 0x00401000;

			for (int ptr = virtual_address; ptr < virtual_address + string_size; ptr++)
			{
				string += buffer[ptr];
			}
			strings.push_back(string);		
		}

	}
	return strings;

}

std::vector<std::string> get_all_user_defined_functions(const std::vector<std::string>& total_funcs)
{
	std::vector<std::string> user_defined_functions;

	for (auto const& func : total_funcs)
	{
		if (func.find("main.") != std::string::npos && func.find("runtime") == std::string::npos)
		{
			user_defined_functions.push_back(split_string(func, ".")[1]);
		}
	}



	return user_defined_functions;
}

void define_main_pkg_funcs(std::vector<Function>* functions, std::vector<std::string>* names)
{
	uint32_t func_it = functions->size() - 1;
	std::vector<std::string> r_names  = *names;
	std::reverse(r_names.begin(), r_names.end());

	for (auto& const name : r_names)
	{
		(*functions)[func_it].function_name = name;
		(*functions)[func_it].from_main_pkg = true;
		func_it--;
	}
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
