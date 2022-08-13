#pragma once
#include <map>
#include <string>

enum ColorSheet {
	Color_Off,
	Black,
	Red,
	Green,
	Yellow,
	Blue,
	Purple,
	Cyan,
	White,
	BBlack,
	BRed,
	BGreen,
	BYellow,
	BBlue,
	BPurple,
	BCyan,
	BWhite,
	UBlack,
	URed,
	UGreen,
	UYellow,
	UBlue,
	UPurple,
	UCyan,
	UWhite,
	On_Black,
	On_Red,
	On_Green,
	On_Yellow,
	On_Blue,
	On_Purple,
	On_Cyan,
	On_White,
	IBlack,
	IRed,
	IGreen,
	IYellow,
	IBlue,
	IPurple,
	ICyan,
	IWhite,
	BIBlack,
	BIRed,
	BIGreen,
	BIYellow,
	BIBlue,
	BIPurple,
	BICyan,
	BIWhite,
	On_IBlack,
	On_IRed,
	On_IGreen,
	On_IYellow,
	On_IBlue,
	On_IPurple,
	On_ICyan,
	On_IWhite,
};

std::map<ColorSheet, std::string> Colors
{
	{ ColorSheet::Color_Off, "\033[0m" },
	{ ColorSheet::Black, "\033[0;30m" },
	{ ColorSheet::Red, "\033[0;31m" },
	{ ColorSheet::Green, "\033[0;32m" },
	{ ColorSheet::Yellow, "\033[0;33m" },
	{ ColorSheet::Blue, "\033[0;34m" },
	{ ColorSheet::Purple, "\033[0;35m" },
	{ ColorSheet::Cyan, "\033[0;36m" },
	{ ColorSheet::White, "\033[0;37m" },
	{ ColorSheet::BBlack, "\033[1;30m" },
	{ ColorSheet::BRed, "\033[1;31m" },
	{ ColorSheet::BGreen, "\033[1;32m" },
	{ ColorSheet::BYellow, "\033[1;33m" },
	{ ColorSheet::BBlue, "\033[1;34m" },
	{ ColorSheet::BPurple, "\033[1;35m" },
	{ ColorSheet::BCyan, "\033[1;36m" },
	{ ColorSheet::BWhite, "\033[1;37m" },
	{ ColorSheet::UBlack, "\033[4;30m" },
	{ ColorSheet::URed, "\033[4;31m" },
	{ ColorSheet::UGreen, "\033[4;32m" },
	{ ColorSheet::UYellow, "\033[4;33m" },
	{ ColorSheet::UBlue, "\033[4;34m" },
	{ ColorSheet::UPurple, "\033[4;35m" },
	{ ColorSheet::UCyan, "\033[4;36m" },
	{ ColorSheet::UWhite, "\033[4;37m" },
	{ ColorSheet::On_Black, "\033[40m" },
	{ ColorSheet::On_Red, "\033[41m" },
	{ ColorSheet::On_Green, "\033[42m" },
	{ ColorSheet::On_Yellow, "\033[43m" },
	{ ColorSheet::On_Blue, "\033[44m" },
	{ ColorSheet::On_Purple, "\033[45m" },
	{ ColorSheet::On_Cyan, "\033[46m" },
	{ ColorSheet::On_White, "\033[47m" },
	{ ColorSheet::IBlack, "\033[0;90m" },
	{ ColorSheet::IRed, "\033[0;91m" },
	{ ColorSheet::IGreen, "\033[0;92m" },
	{ ColorSheet::IYellow, "\033[0;93m" },
	{ ColorSheet::IBlue, "\033[0;94m" },
	{ ColorSheet::IPurple, "\033[0;95m" },
	{ ColorSheet::ICyan, "\033[0;96m" },
	{ ColorSheet::IWhite, "\033[0;97m" },
	{ ColorSheet::BIBlack, "\033[1;90m" },
	{ ColorSheet::BIRed, "\033[1;91m" },
	{ ColorSheet::BIGreen, "\033[1;92m" },
	{ ColorSheet::BIYellow, "\033[1;93m" },
	{ ColorSheet::BIBlue, "\033[1;94m" },
	{ ColorSheet::BIPurple, "\033[1;95m" },
	{ ColorSheet::BICyan, "\033[1;96m" },
	{ ColorSheet::BIWhite, "\033[1;97m" },
	{ ColorSheet::On_IBlack, "\033[0;100m" },
	{ ColorSheet::On_IRed, "\033[0;101m" },
	{ ColorSheet::On_IGreen, "\033[0;102m" },
	{ ColorSheet::On_IYellow, "\033[0;103m" },
	{ ColorSheet::On_IBlue, "\033[0;104m" },
	{ ColorSheet::On_IPurple, "\033[0;105m" },
	{ ColorSheet::On_ICyan, "\033[0;106m" },
	{ ColorSheet::On_IWhite, "\033[0;107m" },
};