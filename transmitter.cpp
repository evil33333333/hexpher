#include "transmitter.h"
#include "helper.h"

std::string translate_function(TMessage* tmsg)
{
	uint32_t start_point = 0x04;
	uint32_t string_count = 0x00;
	uint32_t int_count = 0x00;
	uint32_t boolean_count = 0x00;
	uint32_t voidptr_count = 0x00;

	std::queue<std::string> stack;
	std::vector<uint32_t> instructions_used;
	std::map<std::string, std::uintptr_t> integer_variables;

	tmsg->instructions_used = &instructions_used;
	std::string psuedo_code = std::format("func {}()\n", tmsg->function->function_name);
	psuedo_code += "{\n";

	for (size_t i = start_point; i < tmsg->function->ibuffers.size(); i++)
	{
		if (std::find(instructions_used.begin(), instructions_used.end(), i) == instructions_used.end())
		{
			if (tmsg->function->ibuffers[i].instruction == Instruction::CALL)
			{
				instructions_used.push_back(i);
				std::uintptr_t func_addr;
				std::stringstream stream;
				std::string func_addr_str = split_string(tmsg->function->ibuffers[i].raw_asm, " ")[1];
				stream << std::hex << func_addr_str;
				stream >> func_addr;
				std::string function_name = find_function_name(tmsg->all_functions, func_addr);


				if (tmsg->function->ibuffers[i + 1].raw_asm.find("mov e") != std::string::npos && tmsg->function->ibuffers[i - 2].instruction == Instruction::LEA)
				{
					instructions_used.push_back(i + 1);
					psuedo_code += std::format("{}data, err := {}(", duplicate_string(tab_string, tmsg->indent_level), function_name);
				}
				else
				{
					psuedo_code += std::format("{}data := {}(", duplicate_string(tab_string, tmsg->indent_level), function_name);
				}
				while (!stack.empty())
				{
					psuedo_code += stack.size() == 1 ? std::format("{}", stack.front()) : std::format("{}, ", stack.front());
					stack.pop();
				}
				psuedo_code += ")\n\n";
			}

			if (tmsg->function->ibuffers[i].raw_asm.find("mov BYTE PTR") != std::string::npos)
			{
				instructions_used.push_back(i);

				int value;
				std::stringstream stream;
				std::string byte_string = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];

				stream << std::hex << byte_string;
				stream >> value;

				if (!tmsg->compress)
				{
					psuedo_code += std::format("{}var boolean_{} bool = {}\n", duplicate_string(tab_string, tmsg->indent_level), boolean_count, value == 1 ? "true" : "false");
					stack.push(std::format("boolean_{}", boolean_count));
				}
				else
				{
					stack.push(value == 1 ? "true" : "false");
				}
				
				boolean_count++;
			}

			if (tmsg->function->ibuffers[i].raw_asm.find("mov DWORD PTR") != std::string::npos && tmsg->function->ibuffers[i - 1].instruction != Instruction::LEA)
			{
				signed int value;
				std::uintptr_t address;
				std::stringstream stream;
				instructions_used.push_back(i);
				std::string integer = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];

				if (is_register(integer))
				{
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}var integer_{} int\n", duplicate_string(tab_string, tmsg->indent_level), int_count);
						std::string int_addr = split_string(split_string(tmsg->function->ibuffers[i].raw_asm, "[")[1], "]")[0];

						if (is_register(int_addr))
						{
							continue;
						}

						stream << std::hex << int_addr;
						stream >> address;

						integer_variables[std::format("integer_{}", int_count)] = address;
						stack.push(std::format("integer_{}", int_count));
					}
				}
				else
				{
					stream << std::hex << integer;
					stream >> value;
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}integer_{} := {}\n", duplicate_string(tab_string, tmsg->indent_level), int_count, value);
						stack.push(std::format("integer_{}", int_count));
					}
					else
					{
						stack.push(std::to_string(value));
					}
					
					stream.clear();
				}
				int_count++;
			}


			if (tmsg->function->ibuffers[i].instruction == Instruction::TEST && tmsg->function->ibuffers[i + 1].instruction == Instruction::JZ)
			{
				instructions_used.push_back(i);
				instructions_used.push_back(i + 1);
				psuedo_code += duplicate_string(tab_string, tmsg->indent_level);
				psuedo_code += "if err != nil {  \n\t\tpanic(err)\n  \t}\n";
			}


			// if else statement
			if (tmsg->function->ibuffers[i].instruction == Instruction::ADC && tmsg->function->ibuffers[i + 1].raw_asm[0] == 'j')
			{

				instructions_used.push_back(i);
				instructions_used.push_back(i + 1);
				if (string_contains(tmsg->function->ibuffers[i].raw_asm, "DWORD PTR"))
				{
					int value;
					std::uintptr_t jmp_addr;

					std::stringstream stream;
					std::string int_string = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];
					stream << std::hex << int_string;
					stream >> value;
					stream.clear();

					std::string jmp_addr_str = split_string(tmsg->function->ibuffers[i + 1].raw_asm, " ")[1];
					stream << std::hex << jmp_addr_str;
					stream >> jmp_addr;

					
					psuedo_code += std::format("{}if data {}= {} ", duplicate_string(tab_string, tmsg->indent_level), Instruction::JNE == tmsg->function->ibuffers[i + 1].instruction ? "!" : "=", value);
					psuedo_code += "{\n";

					tmsg->indent_level++;
					tmsg->jmp_addr = jmp_addr;
					psuedo_code += translate_block(tmsg);
					psuedo_code += " else {\n\t";
					continue;

				}
			}

			// this could be an [if, else if, else] or this could be a switch block
			if (tmsg->function->ibuffers[i].instruction == Instruction::ADC && tmsg->function->ibuffers[i + 1].raw_asm[0] == 'j')
			{
				bool first_if_present = false;
				for (;;)
				{
					instructions_used.push_back(i);
					instructions_used.push_back(i + 1);
					if (tmsg->function->ibuffers[i].raw_asm.find("adc e") != std::string::npos)
					{
						int value;
						std::uintptr_t jmp_addr;

						std::stringstream stream;
						std::string int_string = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];
						stream << std::hex << int_string;
						stream >> value;
						stream.clear();

						std::string jmp_addr_str = split_string(tmsg->function->ibuffers[i + 1].raw_asm, " ")[1];
						stream << std::hex << jmp_addr_str;

						stream >> jmp_addr;

						if (!first_if_present)
						{
							psuedo_code += std::format("{}if data {}= {} ", duplicate_string(tab_string, tmsg->indent_level), Instruction::JNE == tmsg->function->ibuffers[i + 1].instruction ? "!" : "=", value);
							psuedo_code += "{\n";
							first_if_present = true;
						}
						else if (tmsg->function->ibuffers[i].instruction == Instruction::ADC && tmsg->function->ibuffers[i + 1].instruction == Instruction::JE || tmsg->function->ibuffers[i + 1].instruction == Instruction::JNE)
						{
							psuedo_code += std::format(" else if data {}= {} ",  Instruction::JNE == tmsg->function->ibuffers[i + 1].instruction ? "!" : "=", value);
							psuedo_code += "{\n";

						}

						
						tmsg->indent_level++;
						tmsg->jmp_addr = jmp_addr;
						psuedo_code += translate_block(tmsg);

						if (tmsg->function->ibuffers[i + 2].instruction == Instruction::ADC)
						{
							i += 2;
						}
						else
						{
							psuedo_code += " else\n"+ duplicate_string(tab_string, tmsg->indent_level) + "{\n" + duplicate_string(tab_string, tmsg->indent_level);
							break;
						}

					}
				}
				
			}

			if (tmsg->function->ibuffers[i].instruction == Instruction::LEA && tmsg->function->ibuffers[i].raw_asm.find(",[e") == std::string::npos && tmsg->function->ibuffers[i + 2].instruction == Instruction::MOV)
			{

				if (tmsg->wrapped_delta != OUT_OF_BOUNDS || tmsg->delta != OUT_OF_BOUNDS)
				{
					int virtual_address = 0;
					int string_size = 0;
					bool found_nested_ptr = false;

					std::string string;
					std::stringstream stream;

					instructions_used.push_back(i);


					std::string string_addr = split_string(split_string(tmsg->function->ibuffers[i].raw_asm, "[")[1], "]")[0];
					stream << std::hex << string_addr;
					stream >> virtual_address;

					stream.clear();

					std::string str_size = split_string(tmsg->function->ibuffers[i + 2].raw_asm, ",")[1];


					instructions_used.push_back(i + 1);
					if (!is_register(str_size))
					{
						if (str_size.find("DWORD PTR") != std::string::npos)
						{

							if (tmsg->wrapped_delta != OUT_OF_BOUNDS)
							{
								virtual_address = virtual_address - tmsg->wrapped_delta;

								std::uintptr_t string_ptr = 0x00;
								std::string _byte;
								std::string text;
								for (int ptr = virtual_address + 0x03; ptr != virtual_address - 0x01; ptr--)
								{
									_byte = tmsg->buffer[ptr];
									text += string_to_hex(_byte);
								}
								std::istringstream ss(text);
								ss >> std::hex >> string_ptr;
								text.clear();

								std::uintptr_t wrapper_str_start = string_ptr - tmsg->wrapped_delta;

								for (int ptr = virtual_address + 0x07; ptr != virtual_address + 0x03; ptr--)
								{
									_byte = tmsg->buffer[ptr];
									text += string_to_hex(_byte);
								}

								std::istringstream dw_string(text);
								dw_string >> std::hex >> string_size;

								if (wrapper_str_start < tmsg->rdata_start || wrapper_str_start > tmsg->b_size || wrapper_str_start + string_size > tmsg->b_size)
								{
									continue;
								}

								for (uintptr_t ptr = wrapper_str_start; ptr < wrapper_str_start + string_size; ptr++)
								{
									string += tmsg->buffer[ptr];
								}

								if (!tmsg->compress)
								{
									psuedo_code += std::format("{}string_{} := \"{}\"\n", duplicate_string(tab_string, tmsg->indent_level), string_count, string);
									stack.push(std::format("string_{}", string_count));
								}
								else
								{
									stack.push(std::format("\"{}\"", string));
								}
								
								string_count++;
							}
							
						}

						else
						{
							if (tmsg->delta != OUT_OF_BOUNDS)
							{
								instructions_used.push_back(i + 2);
								stream << std::hex << str_size;
								stream >> string_size;
								stream.clear();

								virtual_address = virtual_address - tmsg->delta;

								for (int ptr = virtual_address; ptr < virtual_address + string_size; ptr++)
								{
									string += tmsg->buffer[ptr];
								}
								if (!tmsg->compress)
								{
									psuedo_code += std::format("{}string_{} := \"{}\"\n", duplicate_string(tab_string, tmsg->indent_level), string_count, string);
									stack.push(std::format("string_{}", string_count));
								}
								else
								{
									stack.push(std::format("\"{}\"", string));
								}
								string_count++;
							}
						}
					}
				}
			}
			if (tmsg->function->ibuffers[i].instruction == Instruction::LEA && contains_element<unsigned int>(tmsg->instructions_used, i))
			{
				std::string addr;
				instructions_used.push_back(i);

				if (tmsg->function->ibuffers[i].raw_asm.find(",[e") == std::string::npos)
				{
					addr = split_string(split_string(tmsg->function->ibuffers[i].raw_asm, "[")[1], "]")[0];
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}var voidptr_{} uintptr = {}\n", duplicate_string(tab_string, tmsg->indent_level), voidptr_count, addr);
					}
				}
				else
				{
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}var voidptr_{} uintptr\n", duplicate_string(tab_string, tmsg->indent_level), voidptr_count);
					}
				}
				if (!tmsg->compress)
				{
					stack.push(std::format("voidptr_{}", voidptr_count));
				}
				else
				{
					stack.push(addr);
				}
				voidptr_count++;
			}
		}
	}
	psuedo_code += "\n}";
	return psuedo_code;
}

std::string translate_block(TMessage* tmsg)
{
	size_t i;
	for (i = 0; i < tmsg->function->ibuffers.size(); i++)
	{
		if (tmsg->function->ibuffers[i].address == tmsg->jmp_addr)
		{
			break;
		}
	}
	uint32_t string_count = 0x00;
	uint32_t int_count = 0x00;
	uint32_t boolean_count = 0x00;

	std::queue<std::string> stack;
	std::map<std::string, std::uintptr_t> integer_variables;
	std::string psuedo_code;

	for (; i < tmsg->function->ibuffers.size(); i++)
	{

		if (std::find(tmsg->instructions_used->begin(), tmsg->instructions_used->end(), i) == tmsg->instructions_used->end())
		{
			if (tmsg->function->ibuffers[i].instruction == Instruction::JMP)
			{
				tmsg->indent_level--;
				return psuedo_code;
			}


			if (tmsg->function->ibuffers[i].instruction == Instruction::CALL)
			{
				tmsg->instructions_used->push_back(i);
				std::uintptr_t func_addr;
				std::stringstream stream;
				std::string func_addr_str = split_string(tmsg->function->ibuffers[i].raw_asm, " ")[1];
				stream << std::hex << func_addr_str;
				stream >> func_addr;
				std::string function_name = find_function_name(tmsg->all_functions, func_addr);


				if (tmsg->function->ibuffers[i + 1].raw_asm.find("mov e") != std::string::npos && tmsg->function->ibuffers[i - 2].instruction == Instruction::LEA)
				{
					tmsg->instructions_used->push_back(i + 1);
					psuedo_code += std::format("{}data, err := {}(", duplicate_string(tab_string, tmsg->indent_level), function_name);
				}
				else
				{
					psuedo_code += std::format("{}data := {}(", duplicate_string(tab_string, tmsg->indent_level), function_name);
				}
				while (!stack.empty())
				{
					psuedo_code += stack.size() == 1 ? std::format("{}", stack.front()) : std::format("{}, ", stack.front());
					stack.pop();
				}
				psuedo_code += ")\n\n\t";
			}

			if (tmsg->function->ibuffers[i].raw_asm.find("mov BYTE PTR") != std::string::npos)
			{
				tmsg->instructions_used->push_back(i);

				int value;
				std::stringstream stream;
				std::string byte_string = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];

				stream << std::hex << byte_string;
				stream >> value;


				psuedo_code += std::format("{}var boolean_{} bool = {}\n", duplicate_string(tab_string, tmsg->indent_level), boolean_count, value == 1 ? "true" : "false");
				stack.push(std::format("boolean_{}", boolean_count));
				boolean_count++;
			}

			if (tmsg->function->ibuffers[i].raw_asm.find("mov DWORD PTR") != std::string::npos && tmsg->function->ibuffers[i - 1].instruction != Instruction::LEA)
			{
				signed int value;
				std::uintptr_t address;
				std::stringstream stream;
				tmsg->instructions_used->push_back(i);
				std::string integer = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];

				if (is_register(integer))
				{
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}var integer_{} int\n", duplicate_string(tab_string, tmsg->indent_level), int_count);
						std::string int_addr = split_string(split_string(tmsg->function->ibuffers[i].raw_asm, "[")[1], "]")[0];

						if (is_register(int_addr))
						{
							continue;
						}

						stream << std::hex << int_addr;
						stream >> address;

						integer_variables[std::format("integer_{}", int_count)] = address;
						stack.push(std::format("integer_{}", int_count));
					}
				}
				else
				{
					stream << std::hex << integer;
					stream >> value;
					if (!tmsg->compress)
					{
						psuedo_code += std::format("{}integer_{} := {}\n", duplicate_string(tab_string, tmsg->indent_level), int_count, value);
						stack.push(std::format("integer_{}", int_count));
					}
					else
					{
						stack.push(std::to_string(value));
					}

					stream.clear();
				}
				int_count++;
			}


			if (tmsg->function->ibuffers[i].instruction == Instruction::TEST && tmsg->function->ibuffers[i + 1].instruction == Instruction::JZ)
			{
				tmsg->instructions_used->push_back(i);
				tmsg->instructions_used->push_back(i + 1);
				psuedo_code += duplicate_string(tab_string, tmsg->indent_level);
				psuedo_code += "if err != nil {  \n\t\tpanic(err)\n  \t}\n";
			}


			if (tmsg->function->ibuffers[i].instruction == Instruction::ADC && tmsg->function->ibuffers[i + 1].raw_asm[0] == 'j')
			{

				tmsg->instructions_used->push_back(i);
				tmsg->instructions_used->push_back(i + 1);
				if (tmsg->function->ibuffers[i].raw_asm.find("DWORD PTR") != std::string::npos)
				{
					int value;
					std::uintptr_t jmp_addr;

					std::stringstream stream;
					std::string int_string = split_string(tmsg->function->ibuffers[i].raw_asm, ",")[1];
					stream << std::hex << int_string;
					stream >> value;
					stream.clear();

					std::string jmp_addr_str = split_string(tmsg->function->ibuffers[i + 1].raw_asm, " ")[1];
					stream << std::hex << jmp_addr_str;
					stream >> jmp_addr;


					psuedo_code += std::format("{}if data {}= {} ", duplicate_string(tab_string, tmsg->indent_level), Instruction::JNE == tmsg->function->ibuffers[i + 1].instruction ? "!" : "=", value);
					psuedo_code += "{\n";

					tmsg->indent_level++;
					
					psuedo_code += translate_block(tmsg);
					psuedo_code += " else {\n\t";
				}
			}

			if (tmsg->function->ibuffers[i].instruction == Instruction::LEA && tmsg->function->ibuffers[i].raw_asm.find(",[e") == std::string::npos && tmsg->function->ibuffers[i + 2].instruction == Instruction::MOV)
			{
				if (tmsg->wrapped_delta != OUT_OF_BOUNDS || tmsg->delta != OUT_OF_BOUNDS)
				{
					int virtual_address = 0;
					int string_size = 0;
					bool found_nested_ptr = false;

					std::string string;
					std::stringstream stream;

					tmsg->instructions_used->push_back(i);


					std::string string_addr = split_string(split_string(tmsg->function->ibuffers[i].raw_asm, "[")[1], "]")[0];
					stream << std::hex << string_addr;
					stream >> virtual_address;

					stream.clear();

					std::string str_size = split_string(tmsg->function->ibuffers[i + 2].raw_asm, ",")[1];


					tmsg->instructions_used->push_back(i + 1);
					if (!is_register(str_size))
					{
						if (str_size.find("DWORD PTR") != std::string::npos)
						{

							if (tmsg->wrapped_delta != OUT_OF_BOUNDS)
							{
								virtual_address = virtual_address - tmsg->wrapped_delta;

								std::uintptr_t string_ptr = 0x00;
								std::string _byte;
								std::string text;
								for (int ptr = virtual_address + 0x03; ptr != virtual_address - 0x01; ptr--)
								{
									_byte = tmsg->buffer[ptr];
									text += string_to_hex(_byte);
								}
								std::istringstream ss(text);
								ss >> std::hex >> string_ptr;
								text.clear();

								std::uintptr_t wrapper_str_start = string_ptr - tmsg->wrapped_delta;

								for (int ptr = virtual_address + 0x07; ptr != virtual_address + 0x03; ptr--)
								{
									_byte = tmsg->buffer[ptr];
									text += string_to_hex(_byte);
								}

								std::istringstream dw_string(text);
								dw_string >> std::hex >> string_size;

								if (wrapper_str_start < tmsg->rdata_start || wrapper_str_start > tmsg->b_size || wrapper_str_start + string_size > tmsg->b_size)
								{
									continue;
								}

								for (uintptr_t ptr = wrapper_str_start; ptr < wrapper_str_start + string_size; ptr++)
								{
									string += tmsg->buffer[ptr];
								}

								if (!tmsg->compress)
								{
									psuedo_code += std::format("{}string_{} := \"{}\"\n", duplicate_string(tab_string, tmsg->indent_level), string_count, string);
									stack.push(std::format("string_{}", string_count));
								}
								else
								{
									stack.push(std::format("\"{}\"", string));
								}

								string_count++;
							}

						}

						else
						{
							if (tmsg->delta != OUT_OF_BOUNDS)
							{
								tmsg->instructions_used->push_back(i + 2);
								stream << std::hex << str_size;
								stream >> string_size;
								stream.clear();

								virtual_address = virtual_address - tmsg->delta;

								for (int ptr = virtual_address; ptr < virtual_address + string_size; ptr++)
								{
									string += tmsg->buffer[ptr];
								}
								if (!tmsg->compress)
								{
									psuedo_code += std::format("{}string_{} := \"{}\"\n", duplicate_string(tab_string, tmsg->indent_level), string_count, string);
									stack.push(std::format("string_{}", string_count));
								}
								else
								{
									stack.push(std::format("\"{}\"", string));
								}
								string_count++;
							}
						}
					}
				}
			}
		}
	}
	psuedo_code += "\n\t}";
	tmsg->indent_level--;
	return psuedo_code;
}

std::string find_function_name(std::vector<Function>* functions, std::uintptr_t x32_addr)
{
	for (auto const& func : *functions)
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

	std::for_each(buffer, buffer + buff_size, [&](char& c) { _buffer.push_back(c); });

	std::string first_gofunc = "__x86.get_pc_thunk.ax";

	std::vector<std::string>::iterator new_pkg;
	std::vector<unsigned char>::iterator start = std::search(_buffer.begin(), _buffer.end(), first_gofunc.begin(), first_gofunc.end());

	bool found_new_pkg = false;
	std::string previous_pkg;
	std::string function_name;
	while (null_count != 1 || *start != 0x00)
	{
		if (*start == 0x00)
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

	return function_names;

}

std::vector<std::string> get_wrapped_strings_from_function(Function* function, char* buffer, std::size_t buffer_size, std::uintptr_t rdata_start, std::uintptr_t wrapped_delta)
{
	std::vector<std::string> strings;
	for (size_t i = 0x04; i < function->ibuffers.size(); i++)
	{
		if (function->ibuffers[i].instruction == Instruction::LEA && function->ibuffers[i + 2].instruction == Instruction::MOV)
		{
			std::uintptr_t virtual_address = 0;
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
				virtual_address = virtual_address - wrapped_delta;
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
				}
				//std::string string_ptr_addr = std::format("{}", string_to_hex(text));
				std::istringstream ss(text);
				ss >> std::hex >> string_ptr;
				text.clear();

				std::uintptr_t wrapper_str_start = string_ptr - wrapped_delta;

				for (std::uintptr_t ptr = virtual_address + 0x07; ptr != virtual_address + 0x03; ptr--)
				{
					_byte = buffer[ptr];
					text += string_to_hex(_byte);
				}

				std::istringstream dw_string(text);
				dw_string >> std::hex >> string_size;

				if (wrapper_str_start < rdata_start || wrapper_str_start > buffer_size || wrapper_str_start + string_size > buffer_size)
				{
					continue;
				}


				for (std::uintptr_t ptr = wrapper_str_start; ptr < wrapper_str_start + string_size; ptr++)
				{
					string += buffer[ptr];
				}
				strings.push_back(string);
			}
		}
		
	}

	return strings;
	
}

std::vector<std::string> get_strings_from_function(Function* function, char* buffer, std::uintptr_t delta)
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
			if (is_register(str_size))
			{
				continue;
			}
			stream << std::hex << str_size;
			stream >> string_size;

			virtual_address = virtual_address - delta;

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
	std::for_each(buffer, buffer + buff_size, [&](char& c) { _buffer.push_back(c); });
	std::vector<unsigned char>::iterator start = std::search(_buffer.begin(), _buffer.end(), std::begin(gofunc_label_start), std::end(gofunc_label_start));

	std::string function_name;
	while (null_count != 1 || *start != 0x00)
	{
		if (function_name == last_func_name)
		{
			function_names.push_back(function_name);
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

	return function_names;
}

void label_all_functions(std::vector<Function>* functions, std::vector<std::string>* names)
{
	uint32_t func_it = functions->size() - 1;
	std::vector<std::string> r_names = *names;
	std::reverse(r_names.begin(), r_names.end());

	for (auto const& name : r_names)
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

std::vector<std::string> find_required_init_placeholders(std::vector<std::string>& names)
{
	std::vector<std::string> placeholders;
	for (size_t i = 0; i < names.size(); i++)
	{
		if (names[i].find("type..eq") != std::string::npos)
		{
			if (names[i - 1].find("init") != std::string::npos)
			{
				placeholders.push_back(names[i]);
			}
		}
	}
	return placeholders;
}

void fix_function_labels(std::vector<std::string>* labels, std::vector<std::string>* names)
{
	std::vector<std::string> required_placeholders = find_required_init_placeholders(*names);
	for (size_t i = 0; i < labels->size(); i++)
	{
		if ((*labels)[i].find("type..eq") != std::string::npos && 
			std::find(required_placeholders.begin(), required_placeholders.end(), (*labels)[i]) != required_placeholders.end())
		{
			if ((*labels)[i - 1].find("init") == std::string::npos && (*labels)[i - 1].find("type..eq") == std::string::npos)
			{
				labels->insert(labels->begin() + (i + 1), "type_init_placeholder");
			}
		}
	}
}

std::vector<std::string> get_main_defined_functions(const std::vector<std::string>& total_funcs)
{
	std::vector<std::string> main_defined_functions;

	for (auto const& func : total_funcs)
	{
		if (func.find("main.") != std::string::npos && func.find("runtime") == std::string::npos)
		{
			main_defined_functions.push_back(split_string(func, ".")[1]);
		}
	}
	return main_defined_functions;
}

void define_main_pkg_funcs(std::vector<Function>* functions, std::vector<std::string>* names)
{
	uint32_t func_it = functions->size() - 1;
	std::vector<std::string> r_names  = *names;
	std::reverse(r_names.begin(), r_names.end());

	for (auto const& name : r_names)
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
	do
	{
		if (!(*functions)[func_it].from_main_pkg)
		{
			(*functions)[func_it].function_name = r_names[name_it];
			name_it--;
		}
		func_it--;
	}
	while (!name_it && !func_it);
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
	std::uintptr_t ret_addr = 0;
	std::uintptr_t buffer_addr = 0;
	std::vector<Function> functions;
	std::vector<unsigned char> _buffer;
	std::for_each(buffer, buffer + buff_size, [&](char& c) { _buffer.push_back(c); });
	

	std::vector<unsigned char>::iterator iter = _buffer.begin();
	do
	{	
		buffer_addr = find_next_function(ret_addr, &_buffer);

		char instructions[0xFF];
		char disassembled[0xFF];
		std::vector<IBuffer> ibuffers;
		std::vector<std::pair<std::string, std::string>> code;
		for (std::uintptr_t i = buffer_addr, count = 0; i < buff_size; i += count)
		{
			count = disassemble((unsigned char*)buffer + i, buff_size - i, i, disassembled);
			instructions[0] = 0;
			for (std::uintptr_t e = 0; e < count; e++)
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

			if (code[code.size() - 1].second.find("ret") != std::string::npos)
			{
				ret_addr = i;
				break;
			}
		}

		if (buffer_addr == OUT_OF_BOUNDS)
		{
			break;
		}
		
		functions.push_back({ buffer_addr, std::format("gofunc_{}", buffer_addr), ibuffers, false });
	}
	while (buffer_addr != OUT_OF_BOUNDS);

	return functions;
}