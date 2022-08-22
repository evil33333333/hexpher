#include "transmitter.h"
#include "helper.h"

std::string translate_function(Function* function, std::vector<Function>* all_functions, char* buffer, size_t b_size, std::uintptr_t rdata_start)
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


	/*s_result println_res = search_for_byte_pattern(&function->instructions, "89 ?? ?? 89 ?? ?? ?? 8d ?? ?? ?? 89 ?? ?? ?? c7 ?? ?? ?? 01 00 00 00 c7 ?? ?? ?? 01 00 00 00 e8");
	if (println_res.found)
	{
		psuedo_code += std::format("\tfmt.Println([unknown_ptr (bad_static_ptr)])\n");
		start_point = println_res.index_found + 1;
	}*/
	

	// Skips the runtime.MoreStackNoCtxt jmp and the sub, rsp [0x??]
	for (size_t i = start_point; i < function->ibuffers.size(); i++)
	{

		if (std::find(instructions_used.begin(), instructions_used.end(), i) == instructions_used.end())
		{

			// runtime.NewObject
			/*
				call    runtime_newobject
				mov     eax, [esp+28h+var_24]
				mov     dword ptr [eax+4], 0x27
				lea     ecx, 0x6B246E
			
			*/
			/*if (function->ibuffers[i].instruction == Instruction::MOV && function->ibuffers[i + 1].instruction == Instruction::LEA && split_string(function->ibuffers[i].raw_asm, ",")[1][0] != 'e')
			{
				

				int value;
				std::uintptr_t address;
				std::string string;

				std::istringstream ss(split_string(function->ibuffers[i].raw_asm, ",")[1]);
				ss >> std::hex >> value;

				std::istringstream str_virtaddr(split_string(split_string(function->ibuffers[i + 1].raw_asm, "[")[1], "]")[0]);
				str_virtaddr >> std::hex >> address;

				address = address - 0x00400C00;
				if (address > rdata_start && address < b_size)
				{
					instructions_used.push_back(i);
					instructions_used.push_back(i + 1);
					for (int ptr = address; ptr < address + value; ptr++)
					{
						string += buffer[ptr];
					}
					psuedo_code += std::format("\tstring_{} := \"{}\"\n", string_count, string);
					stack.push(std::format("string_{}", string_count));
					string_count++;
					continue;
				}


				
			}*/

			if (function->ibuffers[i].instruction == Instruction::CALL)
			{

				instructions_used.push_back(i);
				std::uintptr_t func_addr;
				std::stringstream stream;
				std::string func_addr_str = split_string(function->ibuffers[i].raw_asm, " ")[1];
				stream << std::hex << func_addr_str;
				stream >> func_addr;
				std::string function_name = find_function_name(all_functions, func_addr);


				if (function->ibuffers[i + 1].raw_asm.find("mov e") != std::string::npos && function->ibuffers[i - 2].instruction == Instruction::LEA)
				{
					instructions_used.push_back(i + 1);
					psuedo_code += std::format("\tdata, err := {}(", function_name);
				}
				else
				{
					psuedo_code += std::format("\tdata := {}(", function_name);
				}
				while (!stack.empty())
				{
					psuedo_code += stack.size() == 1 ? std::format("{}", stack.front()) : std::format("{}, ", stack.front());
					stack.pop();
				}
				psuedo_code += ")\n";
			}
			//psuedo_code += std::format("\t{}\n", function->instructions[i].second);

			if (function->ibuffers[i].raw_asm.find("mov BYTE PTR") != std::string::npos)
			{
				instructions_used.push_back(i);

				int value;
				std::stringstream stream;
				std::string byte_string = split_string(function->ibuffers[i].raw_asm, ",")[1];

				stream << std::hex << byte_string;
				stream >> value;


				psuedo_code += std::format("\tvar boolean_{} bool = {}\n", boolean_count, value == 1 ? "true" : "false");
				stack.push(std::format("boolean_{}", boolean_count));
				boolean_count++;
			}

			if (function->ibuffers[i].raw_asm.find("mov DWORD PTR") != std::string::npos && function->ibuffers[i - 1].instruction != Instruction::LEA)
			{
				signed int value;
				std::uintptr_t address;
				std::stringstream stream;
				instructions_used.push_back(i);
				std::string integer = split_string(function->ibuffers[i].raw_asm, ",")[1];

				if (integer[0] == 'e')
				{
					psuedo_code += std::format("\tvar integer_{} int\n", int_count);
					std::string int_addr = split_string(split_string(function->ibuffers[i].raw_asm, "[")[1], "]")[0];

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


			if (function->ibuffers[i].instruction == Instruction::TEST && function->ibuffers[i + 1].instruction == Instruction::JZ)
			{
				instructions_used.push_back(i);
				instructions_used.push_back(i + 1);
				psuedo_code += "\tif err != nil {  \n\t\tpanic(err)\n  \t}\n";
			}


			if (function->ibuffers[i].instruction == Instruction::ADD)
			{
				if (function->ibuffers[i].raw_asm.find("BYTE PTR") != std::string::npos)
				{
					char byte;
					instructions_used.push_back(i);
					std::stringstream stream;
					std::string bytestr = split_string(function->ibuffers[i].raw_asm, ",")[1];
					stream << std::hex << bytestr;

					stream >> byte;

					psuedo_code += std::format("\tif data[0] == \"{}\"", byte);
					psuedo_code += " { BETA_CMP_BLOCK }\n";
				}
			}


			if (function->ibuffers[i].instruction == Instruction::LEA && function->ibuffers[i].raw_asm.find(",[e") == std::string::npos && function->ibuffers[i + 2].instruction == Instruction::MOV)
			{
				int virtual_address = 0;
				int string_size = 0;
				bool found_nested_ptr = false;

				std::string string;
				std::stringstream stream;

				instructions_used.push_back(i);
				

				std::string string_addr = split_string(split_string(function->ibuffers[i].raw_asm, "[")[1], "]")[0];
				stream << std::hex << string_addr;
				stream >> virtual_address;

				stream.clear();

				std::string str_size = split_string(function->ibuffers[i + 2].raw_asm, ",")[1];


				instructions_used.push_back(i + 1);
				if (str_size[0] != 'e')
				{

					// potentially a wrapped string
					if (str_size.find("DWORD PTR") != std::string::npos)
					{
						virtual_address = virtual_address - 0x00400E00;

						// Check for dword ptr (Go may wrap strings in two ptrs [idk why])
						std::uintptr_t string_ptr = 0x00;
						// get first 4 bytes of string
						std::string _byte;
						std::string text;
						for (int ptr = virtual_address + 0x03; ptr != virtual_address - 0x01; ptr--)
						{
							_byte = buffer[ptr];
							text += string_to_hex(_byte);
							//std::memset(reinterpret_cast<void*>(&string_ptr + (ptr + virtual_address)), buffer[ptr], sizeof(char));
						}
						//std::string string_ptr_addr = std::format("{}", string_to_hex(text));
						std::istringstream ss(text);
						ss >> std::hex >> string_ptr;
						text.clear();

						std::uintptr_t wrapper_str_start = string_ptr - 0x00400E00;

						for (int ptr = virtual_address + 0x07; ptr != virtual_address + 0x03; ptr--)
						{
							_byte = buffer[ptr];
							text += string_to_hex(_byte);
							//std::memset(reinterpret_cast<void*>(&string_ptr + (ptr + virtual_address)), buffer[ptr], sizeof(char));
						}

						std::istringstream dw_string(text);
						dw_string >> std::hex >> string_size;

						if (wrapper_str_start < rdata_start || wrapper_str_start > b_size || wrapper_str_start + string_size > b_size)
						{
							continue;
						}

						
						for (int ptr = wrapper_str_start; ptr < wrapper_str_start + string_size; ptr++)
						{
							string += buffer[ptr];
						}
						psuedo_code += std::format("\tstring_{} := \"{}\"\n", string_count, string);
						stack.push(std::format("string_{}", string_count));
						string_count++;
					}

					else
					{
						instructions_used.push_back(i + 2);
						stream << std::hex << str_size;
						stream >> string_size;
						stream.clear();

						virtual_address = virtual_address - 0x00400C00;

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
	}
	psuedo_code += "\n}";
	return psuedo_code;
}

std::string find_function_name(std::vector<Function>* functions, std::uintptr_t x32_addr)
{
	for (auto& const func : *functions)
	{
		if (x32_addr == func.start_addr - 7)
		{
			return func.function_name;
		}
	}
	return std::format("gofunc_{}", x32_addr);
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

std::vector<std::string> get_wrapped_strings_from_function(Function* function, char* buffer, std::size_t buffer_size, std::uintptr_t rdata_start)
{
	std::vector<std::string> strings;
	for (size_t i = 0x04; i < function->ibuffers.size(); i++)
	{
		if (function->ibuffers[i].instruction == Instruction::LEA && function->ibuffers[i + 2].instruction == Instruction::MOV)
		{
			int virtual_address = 0;
			int string_size = 0;

			std::string string;
			std::stringstream stream;

			std::string string_addr = split_string(split_string(function->ibuffers[i].raw_asm, "[")[1], "]")[0];
			stream << std::hex << string_addr;
			stream >> virtual_address;

			stream.clear();

			std::string str_size = split_string(function->ibuffers[i + 2].raw_asm, ",")[1];
			if (str_size.find("DWORD PTR") != std::string::npos && str_size[0] != 'e')
			{
				virtual_address = virtual_address - 0x00400E00;
				if (virtual_address < 0)
				{
					continue;
				}
				// Check for dword ptr (Go may wrap strings in two ptrs [idk why])
				std::uintptr_t string_ptr = 0x00;
				// get first 4 bytes of string
				std::string _byte;
				std::string text;
				for (int ptr = virtual_address + 0x03; ptr != virtual_address - 0x01; ptr--)
				{
					_byte = buffer[ptr];
					text += string_to_hex(_byte);
					//std::memset(reinterpret_cast<void*>(&string_ptr + (ptr + virtual_address)), buffer[ptr], sizeof(char));
				}
				//std::string string_ptr_addr = std::format("{}", string_to_hex(text));
				std::istringstream ss(text);
				ss >> std::hex >> string_ptr;
				text.clear();

				std::uintptr_t wrapper_str_start = string_ptr - 0x00400E00;

				for (int ptr = virtual_address + 0x07; ptr != virtual_address + 0x03; ptr--)
				{
					_byte = buffer[ptr];
					text += string_to_hex(_byte);
					//std::memset(reinterpret_cast<void*>(&string_ptr + (ptr + virtual_address)), buffer[ptr], sizeof(char));
				}

				std::istringstream dw_string(text);
				dw_string >> std::hex >> string_size;

				if (wrapper_str_start < rdata_start || wrapper_str_start > buffer_size || wrapper_str_start + string_size > buffer_size)
				{
					continue;
				}


				for (int ptr = wrapper_str_start; ptr < wrapper_str_start + string_size; ptr++)
				{
					string += buffer[ptr];
				}
				strings.push_back(string);
			}
		}
		
	}

	return strings;
	
}

std::vector<std::string> get_strings_from_function(Function* function, char* buffer)
{
	std::vector<std::string> strings;
	for (size_t i = 0x04; i < function->ibuffers.size(); i++)
	{

		if (function->ibuffers[i].instruction == Instruction::LEA && function->ibuffers[i + 2].instruction == Instruction::MOV)
		{
			int virtual_address = 0;
			int string_size = 0;

			std::string string;
			std::stringstream stream;

			std::string string_addr = split_string(split_string(function->ibuffers[i].raw_asm, "[")[1], "]")[0];
			stream << std::hex << string_addr;
			stream >> virtual_address;

			stream.clear();

			std::string str_size = split_string(function->ibuffers[i + 2].raw_asm, ",")[1];
			if (str_size[0] == 'e')
			{
				continue;
			}
			stream << std::hex << str_size;
			stream >> string_size;

			virtual_address = virtual_address - 0x00400C00;

			for (int ptr = virtual_address; ptr < virtual_address + string_size; ptr++)
			{
				string += buffer[ptr];
			}
			strings.push_back(string);		
		}

	}
	return strings;

}

std::vector<std::string> get_all_function_labels(char* buffer, size_t buff_size, std::string last_func_name)
{
	int null_count = 0;
	std::vector<unsigned char> _buffer;
	std::vector<std::string> function_names;
	populate_buffer(&_buffer, buffer, buff_size);

	// Where the gofunc label ending table starts
	std::vector<unsigned char>::iterator start = std::search(_buffer.begin(), _buffer.end(), std::begin(gofunc_label_start), std::end(gofunc_label_start));

	std::string function_name;
	for (;;)
	{
		if (null_count == 1 && *start == 0x00)
		{
			break;
		}

		else if (function_name == last_func_name)
		{
			function_names.push_back(function_name);
			
			break;
		}
		else if (*start == 0x00)
		{
			function_names.push_back(function_name);
			if (function_name.find("type..eq") != std::string::npos)
			{
				function_names.insert(function_names.begin() + (function_names.size() - 1), "type_init_placeholder");
			}
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

	return function_names;
}

void label_all_functions(std::vector<Function>* functions, std::vector<std::string>* names)
{
	uint32_t func_it = functions->size() - 1;
	std::vector<std::string> r_names = *names;
	std::reverse(r_names.begin(), r_names.end());

	for (auto& const name : r_names)
	{
		if (!func_it)
		{
			break;
		}
		(*functions)[func_it].function_name = name;
		if (name.find("main.") != std::string::npos && name.find("runtime.main") == std::string::npos)
		{
			(*functions)[func_it].from_main_pkg = true;
		}
		
		func_it--;
	}
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

std::vector<std::string> get_other_defined_functions(const std::vector<std::string>& total_funcs)
{
	std::vector<std::string> other_defined_functions;

	for (auto const& func : total_funcs)
	{
		bool is_bad_function = (std::count(func.begin(), func.end(), '.') == 2) && func.find("(") == std::string::npos;
		bool no_junk = (func.find("Sx") == std::string::npos) && (func.find("Bx") == std::string::npos);
		if (func.find("main.") == std::string::npos && !is_bad_function && no_junk)
		{
			other_defined_functions.push_back(func);
		}
	}
	return other_defined_functions;
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

void define_other_pkg_funcs(std::vector<Function>* functions, std::vector<std::string>* names)
{
	uint32_t func_it = functions->size() - 1;
	std::vector<std::string> r_names = *names;
	uint32_t name_it = r_names.size() - 2;

	for (;;)
	{
		if (!name_it || !func_it)
		{
			break;
		}

		if (!(*functions)[func_it].from_main_pkg)
		{
			(*functions)[func_it].function_name = r_names[name_it];
			name_it--;
		}
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

Instruction get_instruction_used(std::string& raw_asm)
{
	std::string instruction_string = split_string(raw_asm, " ")[0];
	return instructions_map[instruction_string];
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
		std::vector<IBuffer> ibuffers;
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
			IBuffer ibuffer;

			ibuffer.address = i;
			ibuffer.raw_bytes = get_raw_bytes(code[code.size() - 1].first);
			ibuffer.raw_asm = disassembled;
			ibuffer.instruction = get_instruction_used(ibuffer.raw_asm);

			ibuffers.push_back(ibuffer);

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
		functions.push_back({ b_addr, std::format("gofunc_{}", b_addr), ibuffers, false });
	}

	return functions;
}