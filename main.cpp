#include <fstream>
#include <iostream>
#include <unordered_map>
#include <format>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "ansi.h"
#include "helper.h"
#include "transmitter.h"


class GUI {
public:
	const std::string cyan_plus = Colors[White] + "[" + Colors[Cyan] + "+" + Colors[White] + "]";

	void configure_console()
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD current_conosle_mode{};
		GetConsoleMode(handle, &current_conosle_mode);
		current_conosle_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
		SetConsoleMode(handle, current_conosle_mode);
	}
};

int main(int argc, char* argv[]) 
{

	GUI gui;
	gui.configure_console();
	if (argc != 2)
	{
		return -1;
	}


	SetConsoleTitleA("hexpher: the go decompiler | developed by lucifers wife");
	f_content fc = get_file_content(argv[1]);
	
	std::vector<std::string> names = get_function_names(fc.buffer, fc.size);
	std::vector<std::string> all_function_labels = get_all_function_labels(fc.buffer, fc.size, names[names.size() - 1]);
	std::vector<pe_section> sections = find_pe_sections(argv[1]);
	std::vector<pe_section>::iterator rdata_section = std::find_if(sections.begin(), sections.end(),
		[](const pe_section& p_sect)
		{
			return p_sect.name.find("rdata") != std::string::npos;
		}
	);


	std::cout << gui.cyan_plus << " Found " << names.size() << " total function names in this Go binary!\n" << std::endl;
	std::vector<std::string> user_funcs = get_all_user_defined_functions(names);
	std::vector<std::string> other_funcs = get_other_defined_functions(names);

	std::string user_def_text;
	for (auto const& func : user_funcs)
	{
		user_def_text += gui.cyan_plus;
		user_def_text += " function: ";
		user_def_text += Colors[Cyan];
		user_def_text += func;
		user_def_text += "()\n";
		user_def_text += Colors[White];
	}
	std::cout << "[" << Colors[Cyan] << "User defined functions" << Colors[White] << "]\n" << user_def_text << Colors[White] << std::endl;
	user_def_text.clear();


	std::vector<std::string> imports = get_all_imports(names);
	


	std::cout << "[" << Colors[Cyan] << "imported packages" << Colors[White] << "]\n";

	std::cout << "import (" << std::endl;
	std::string imports_text;
	for (auto const& _import : imports)
	{
		imports_text += std::format("\t\"{}{}{}\"\n", Colors[Cyan], _import, Colors[White]);
	}
	imports_text += ")\n";
	std::cout << imports_text << std::endl;

	std::vector<Function> funcs = get_all_go_functions(fc.buffer, fc.size);
	label_all_functions(&funcs, &all_function_labels);
	//define_main_pkg_funcs(&funcs, &user_funcs);
	//define_other_pkg_funcs(&funcs, &other_funcs);

	

	std::cout << "[" << Colors[Cyan] << "string content" << Colors[White] << "]";
	std::string string_content_text;
	for (uint32_t i = 0; i < funcs.size(); i++)
	{
		if (funcs[i].from_main_pkg)
		{
			std::vector<std::string> strings = get_strings_from_function(&funcs[i], fc.buffer);
			
			for (auto const& str : strings)
			{
				string_content_text += gui.cyan_plus;
				string_content_text += " string: \"";
				string_content_text += Colors[Cyan];
				string_content_text += str;
				string_content_text += Colors[White];
				string_content_text += "\"\n";
				string_content_text += Colors[White];
			}
			
		}
	}
	std::cout << std::endl << string_content_text << std::endl << std::endl;


	std::cout << "[" << Colors[Cyan] << "wrapped string content" << Colors[White] << "]";
	std::string ws_content_text;
	for (uint32_t i = 0; i < funcs.size(); i++)
	{
		if (funcs[i].from_main_pkg)
		{
			std::vector<std::string> strings = get_wrapped_strings_from_function(&funcs[i], fc.buffer, fc.size, rdata_section->raw_address);

			for (auto const& str : strings)
			{
				ws_content_text += gui.cyan_plus;
				ws_content_text += " string: \"";
				ws_content_text += Colors[Cyan];
				ws_content_text += str;
				ws_content_text += Colors[White];
				ws_content_text += "\"\n";
				ws_content_text += Colors[White];
			}

		}
	}
	std::cout << std::endl << ws_content_text << std::endl << std::endl;

	
	for (uint32_t i = 0; i < funcs.size(); i++)
	{
		if (funcs[i].from_main_pkg)
		{
			std::string psuedo_code = translate_function(&funcs[i], &funcs, fc.buffer, fc.size, rdata_section->raw_address);
			std::cout << psuedo_code << std::endl << std::endl;
		}
	}
	

	std::cin.get();
	return 0;
}


