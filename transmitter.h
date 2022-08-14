#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <format>
#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <queue>

extern "C" {
#include "disassembler.h"
}

struct Function 
{
	std::uintptr_t start_addr;
	std::string function_name;
	std::vector<std::pair<std::string, std::string>> instructions;
	bool from_main_pkg = false;
};

// Every 32 bit Go binary starts a function with these bytes
/*

64 -> fs
8B -> mov instruction
0D -> ecx 
14 00 00 00 -> 0x14

IDA View:

 mov ecx, large fs:14h

*/

inline unsigned char gofunc_bytes[] = {0x64, 0x8B, 0x0D, 0x14, 0x00, 0x00, 0x00};
inline unsigned char gofunc_label_terminator[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//inline std::array<std::string, 44> not_gofuncs = {
//	"__x86.get_pc_thunk", "go.buildid", "interal/cpugetGOAMD64level", "internal/cpu.xgetbv", "internal/cpu.cpuid",
//	"internal/cpu.isSet", "internal/cpu.indexByte", "runtime/internal/atomic.LoadAcq", "runtime/internal/atomic.LoadAcquintptr",
//	"runtime/internal/atomic.Load8", "runtime/internal/atomic.Load","runtime/internal/atomic.Loadp", "runtime/internal/atomic.Cas",
//	"runtime/internal/atomic.Casint64", "runtime/internal/atomic.Casuintptr", "runtime/internal/atomic.CasRel", "runtime/internal/atomic.Loaduintptr",
//	"runtime/internal/atomic.Loaduint", "runtime/internal/atomic.Storeint32", "runtime/internal/atomic.Storeint64", "runtime/internal/atomic.Storeuintptr",
//"runtime/internal/atomic.Xadduintptr",
//"runtime/internal/atomic.Loadint32",
//"runtime/internal/atomic.Loadint64",
//"runtime/internal/atomic.Xaddint64",
//"runtime/internal/atomic.Cas64",
//"runtime/internal/atomic.Casp1",
//"runtime/internal/atomic.Xadd",
//"runtime/internal/atomic.Xadd64",
//"runtime/internal/atomic.Xchg",
//"runtime/internal/atomic.Xchguintptr",
//"runtime/internal/atomic.Xchg64",
//"runtime/internal/atomic.StorepNoWB",
//"runtime/internal/atomic.Store",
//"runtime/internal/atomic.StoreRel",
//"runtime/internal/atomic.StoreReluintptr",
//"runtime/internal/atomic.Load64",
//"runtime/internal/atomic.Store64",
//"runtime/internal/atomic.Or8",
//"runtime/internal/atomic.And8",
//"runtime/internal/atomic.Store8",
//"runtime/internal/atomic.Or",
//"runtime/internal/atomic.And",
//"type..eq.runtime/internal/atomic.Uint64",
//};


std::string translate_function(Function* function, char* buffer);
std::vector<std::string> get_function_names(char* buffer, size_t buff_size);
std::vector<Function> get_all_go_functions(char* buffer, size_t buff_size);
std::vector<std::string> get_strings_from_function(Function* function, char* buffer);
std::vector<std::string> get_all_user_defined_functions(const std::vector<std::string>& total_funcs);
std::vector<std::string> get_all_imports(const std::vector<std::string>& func_names);
void define_main_pkg_funcs(std::vector<Function>* functions, std::vector<std::string>* names);