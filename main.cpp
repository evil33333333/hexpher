#include <fstream>
#include <iostream>
#include <unordered_map>
#include <boost/format.hpp>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "ansi.h"
#include "helper.h"
#include "transmitter.h"
#include "gui.hpp"


int main(int argc, char** argv)
{
	GUI gui;
	std::uintptr_t delta;
	std::uintptr_t wrapped_delta;
	bool arg_compress;

	gui.configure_console();
	if (argc != 5)
	{
		std::cout << gui.purple_plus << " usage: <hexpher> <exe_path> <delta [hex]> <wrapped_delta [hxd]> <arg_compress <dec>>" << std::endl;
		return -1;
	}

	SetConsoleTitleA("hexpher: the go decompiler | developed by lucifers wife");
	f_content fc = get_file_content(argv[1]);

	if (fc.buffer[0x84] != 0x4C)
	{
		std::cout << gui.purple_plus << " sorry, the file provided was not detected as a go binary and/or 32-bit :(" << std::endl;
		return -1;
	}


	std::istringstream i_base(argv[2]);
	std::istringstream wi_base(argv[3]);
	std::istringstream arg_comp(argv[4]);

	i_base >> std::hex >> delta;
	wi_base >> std::hex >> wrapped_delta;
	arg_comp >> std::dec >> arg_compress;


	std::vector<std::string> names = get_function_names(fc.buffer, fc.size);
	std::string last_func = find_last_function(&names);
	std::vector<std::string> all_function_labels = get_all_function_labels(fc.buffer, fc.size, last_func);
	fix_function_labels(&all_function_labels, &names);
	std::vector<pe_section> sections = find_pe_sections(argv[1]);
	std::vector<pe_section>::iterator rdata_section = std::find_if(sections.begin(), sections.end(),
		[](const pe_section& p_sect)
	{
		return p_sect.name.find("rdata") != std::string::npos;
	}
	);


	std::cout << gui.cyan_plus << " Found " << all_function_labels.size() << " total function names in this Go binary!\n" << std::endl;
	std::vector<std::string> main_funcs = get_main_defined_functions(names);

	std::string main_def_text;
	for (auto const& func : main_funcs)
	{
		auto fmt = boost::format("%1% function: %2%%3%()\n%4%") % gui.cyan_plus % Colors[Cyan] % func % Colors[White];
		main_def_text += fmt.str();
	}
	std::cout << "[" << Colors[Cyan] << "main pkg functions" << Colors[White] << "]\n" << main_def_text << Colors[White] << std::endl;
	main_def_text.clear();

	std::vector<std::string> packages = get_all_imports(names);

	std::cout << "[" << Colors[Cyan] << "packages" << Colors[White] << "]\n";

	std::cout << "import (" << std::endl;
	std::string imports_text;
	for (auto const& pkg : packages)
	{
		auto fmt = boost::format("\t\"%1%%2%%3%\"\n") % Colors[Cyan] % pkg % Colors[White];
		imports_text += fmt.str();
	}
	imports_text += ")\n";
	std::cout << imports_text << std::endl;

	std::vector<Function> funcs = get_all_go_functions(fc.buffer, fc.size);
	label_all_functions(&funcs, &all_function_labels);

	if (delta != OUT_OF_BOUNDS)
	{
		std::cout << "[" << Colors[Cyan] << "strings" << Colors[White] << "]";
		std::string string_content_text;
		for (uint32_t i = 0; i < funcs.size(); i++)
		{
			if (funcs[i].from_main_pkg)
			{
				std::vector<std::string> strings = get_strings_from_function(&funcs[i], fc.buffer, delta);

				for (auto const& str : strings)
				{
					auto fmt = boost::format("%1% \"%2%%3%%4%\"\n%5%") % gui.cyan_plus % Colors[Cyan] % str % Colors[White] % Colors[White];
					string_content_text += fmt.str();
				}
			}
		}
		std::cout << std::endl << string_content_text << std::endl;
	}


	if (wrapped_delta != OUT_OF_BOUNDS)
	{
		//std::cout << "[" << Colors[Cyan] << "wrapped string content" << Colors[White] << "]";
		std::string ws_content_text;
		for (uint32_t i = 0; i < funcs.size(); i++)
		{
			if (funcs[i].from_main_pkg)
			{
				std::vector<std::string> strings = get_wrapped_strings_from_function(&funcs[i], fc.buffer, fc.size, rdata_section->raw_address, wrapped_delta);

				for (auto const& str : strings)
				{
					auto fmt = boost::format("%1% \"%2%%3%%4%\"\n%5%") % gui.cyan_plus % Colors[Cyan] % str % Colors[White] % Colors[White];
					ws_content_text += fmt.str();
				}
			}
		}
		std::cout << std::endl << ws_content_text << std::endl << std::endl;
	}


	for (uint32_t i = 0; i < funcs.size(); i++)
	{
		if (funcs[i].from_main_pkg)
		{
			TMessage tmsg = { 0 };
			tmsg.function = &funcs[i];
			tmsg.all_functions = &funcs;
			tmsg.buffer = fc.buffer;
			tmsg.b_size = fc.size;
			tmsg.rdata_start = rdata_section->raw_address;
			tmsg.delta = delta;
			tmsg.wrapped_delta = wrapped_delta;
			tmsg.compress = arg_compress;

			std::string psuedo_code = translate_function(&tmsg);
			std::cout << psuedo_code << std::endl << std::endl;
		}
	}
	std::cin.get();
	return 0;
}


