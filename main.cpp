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
};

int main(int argc, char* argv[]) 
{

	GUI gui;

	if (argc != 2)
	{
		return -1;
	}


	SetConsoleTitleA("hexpher: the go decompiler | developed by lucifers wife");
	FILE* file = fopen(argv[1], "rb");
	if (!file)
	{
		return -1;
	}

	fseek(file, 0L, SEEK_END);
	long numbytes = ftell(file);
	fseek(file, 0L, SEEK_SET);
	char* bytes = (char*)calloc(numbytes, sizeof(char));

	if (!bytes)
	{
		return -1;
	}

	fread(bytes, sizeof(char), numbytes, file);
	fclose(file);
	
	std::vector<std::string> names = get_function_names(bytes, numbytes);
	


	std::cout << gui.cyan_plus << " Found " << names.size() << " total function names in this Go binary!\n" << std::endl;
	std::vector<std::string> user_funcs = get_all_user_defined_functions(names);

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

	std::cout << "[" << Colors[Cyan] << "function content" << Colors[White] << "]";
	std::vector<Function> funcs = get_all_go_functions(bytes, numbytes);

	for (auto& func : funcs)
	{
		std::string code = translate_function(&func);
		std::cout << code << std::endl << std::endl;
	}
	
	std::cin.get();
	return 0;
}


