#include <windows.h>
#include "ansi.h"

class GUI {
public:
	const std::string cyan_plus = Colors[White] + "[" + Colors[Cyan] + "+" + Colors[White] + "]";
	const std::string purple_plus = Colors[White] + "[" + Colors[Purple] + "+" + Colors[White] + "]";

	void configure_console()
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD current_conosle_mode{};
		GetConsoleMode(handle, &current_conosle_mode);
		current_conosle_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
		SetConsoleMode(handle, current_conosle_mode);
	}
};