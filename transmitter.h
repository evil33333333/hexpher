#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/format.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <queue>
#include <chrono>

extern "C" {
#include "disassembler.h"
}

enum Instruction
{
	AAA,
	AAD,
	AAM,
	AAS,
	ADC,
	ADD,
	AND,
	CALL,
	CBW,
	CLC,
	CLD,
	CLI,
	CMC,
	CMP,
	CMPSB,
	CMPSW,
	CWD,
	DAA,
	DAS,
	DEC,
	DIV,
	ESC,
	HLT,
	IDIV,
	IMUL,
	_IN,
	INC,
	_INT,
	INTO,
	IRET,
	JA,
	JAE,
	JB,
	JBE,
	JC,
	JE,
	JG,
	JGE,
	JL,
	JLE,
	JNA,
	JNAE,
	JNB,
	JNBE,
	JNC,
	JNE,
	JNG,
	JNGE,
	JNL,
	JNLE,
	JNO,
	JNP,
	JNS,
	JNZ,
	JO,
	JP,
	JPE,
	JPO,
	JS,
	JZ,
	JCXZ,
	JMP,
	LAHF,
	LDS,
	LEA,
	LES,
	LOCK,
	LODSB,
	LODSW,
	LOOP,
	MOV,
	MOVSB,
	MOVSW,
	MUL,
	NEG,
	NOP,
	NOT,
	OR,
	_OUT,
	POP,
	POPF,
	PUSH,
	PUSHF,
	RCL,
	RCR,
	REP,
	REPE,
	REPNE,
	REPNZ,
	REPZ,
	RET,
	RETN,
	RETF,
	ROL,
	ROR,
	SAHF,
	SAL,
	SAR,
	SBB,
	SCASB,
	SCASW,
	SHL,
	SHR,
	STC,
	STD,
	STI,
	STOSB,
	STOSW,
	SUB,
	TEST,
	WAIT,
	XCHG,
	XLAT,
	XOR,
	UNDEFINED

};

struct IBuffer
{
	std::string raw_asm;
	Instruction instruction = Instruction::UNDEFINED;
	std::vector<std::string> raw_bytes;
	std::uintptr_t address = 0xffffffff;
};


struct Function
{
	std::uintptr_t start_addr;
	std::string function_name;
	std::vector<IBuffer> ibuffers;
	bool from_main_pkg = false;
};

struct TMessage
{
	Function* function;
	std::vector<Function>* all_functions;
	std::uintptr_t jmp_addr = 0xFFFFFFFF;
	char* buffer = nullptr;
	size_t b_size = -1;
	std::uintptr_t rdata_start = 0xFFFFFFFF;
	std::uintptr_t delta = 0xFFFFFFFF;
	std::uintptr_t wrapped_delta = 0xFFFFFFFF;
	std::vector<uint32_t>* instructions_used = nullptr;
	int indent_level = 1;
	bool compress = false;
};

// Every 32 bit Go binary starts a function with these bytes
/*

64 -> fs
8B -> mov instruction
0D -> ecx
140x00,0x00,0x00, -> 0x14

IDA View:

mov ecx, large fs:14h

*/

static const unsigned char gofunc_bytes[] = { 0x64, 0x8B, 0x0D, 0x14, 0x00, 0x00, 0x00 };
static const unsigned char gofunc_label_terminator[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const unsigned char gofunc_label_start[] = { 0x74, 0x00, 0x72, 0x75, 0x6E, 0x74, 0x69, 0x6D, 0x65, 0x2E, 0x65, 0x74, 0x65, 0x78, 0x74, 0x00 };


const std::string tab_string = "\t";

static const std::map<std::string, Instruction> instructions_map
{
	{ "aaa", AAA },
	{ "aad", AAD },
	{ "aam", AAM },
	{ "aas", AAS },
	{ "adc", ADC },
	{ "add", ADD },
	{ "and", AND },
	{ "call", CALL },
	{ "cbw", CBW },
	{ "clc", CLC },
	{ "cld", CLD },
	{ "cli", CLI },
	{ "cmc", CMC },
	{ "cmp", CMP },
	{ "cmpsb", CMPSB },
	{ "cmpsw", CMPSW },
	{ "cwd", CWD },
	{ "daa", DAA },
	{ "das", DAS },
	{ "dec", DEC },
	{ "div", DIV },
	{ "esc", ESC },
	{ "hlt", HLT },
	{ "idiv", IDIV },
	{ "imul", IMUL },
	{ "_in", _IN },
	{ "inc", INC },
	{ "_int", _INT },
	{ "into", INTO },
	{ "iret", IRET },
	{ "ja", JA },
	{ "jae", JAE },
	{ "jb", JB },
	{ "jbe", JBE },
	{ "jc", JC },
	{ "je", JE },
	{ "jg", JG },
	{ "jge", JGE },
	{ "jl", JL },
	{ "jle", JLE },
	{ "jna", JNA },
	{ "jnae", JNAE },
	{ "jnb", JNB },
	{ "jnbe", JNBE },
	{ "jnc", JNC },
	{ "jne", JNE },
	{ "jng", JNG },
	{ "jnge", JNGE },
	{ "jnl", JNL },
	{ "jnle", JNLE },
	{ "jno", JNO },
	{ "jnp", JNP },
	{ "jns", JNS },
	{ "jnz", JNZ },
	{ "jo", JO },
	{ "jp", JP },
	{ "jpe", JPE },
	{ "jpo", JPO },
	{ "js", JS },
	{ "jz", JZ },
	{ "jcxz", JCXZ },
	{ "jmp", JMP },
	{ "lahf", LAHF },
	{ "lds", LDS },
	{ "lea", LEA },
	{ "les", LES },
	{ "lock", LOCK },
	{ "lodsb", LODSB },
	{ "lodsw", LODSW },
	{ "loop", LOOP },
	{ "mov", MOV },
	{ "movsb", MOVSB },
	{ "movsw", MOVSW },
	{ "mul", MUL },
	{ "neg", NEG },
	{ "nop", NOP },
	{ "not", NOT },
	{ "or", OR },
	{ "_out", _OUT },
	{ "pop", POP },
	{ "popf", POPF },
	{ "push", PUSH },
	{ "pushf", PUSHF },
	{ "rcl", RCL },
	{ "rcr", RCR },
	{ "rep", REP },
	{ "repe", REPE },
	{ "repne", REPNE },
	{ "repnz", REPNZ },
	{ "repz", REPZ },
	{ "ret", RET },
	{ "retn", RETN },
	{ "retf", RETF },
	{ "rol", ROL },
	{ "ror", ROR },
	{ "sahf", SAHF },
	{ "sal", SAL },
	{ "sar", SAR },
	{ "sbb", SBB },
	{ "scasb", SCASB },
	{ "scasw", SCASW },
	{ "shl", SHL },
	{ "shr", SHR },
	{ "stc", STC },
	{ "std", STD },
	{ "sti", STI },
	{ "stosb", STOSB },
	{ "stosw", STOSW },
	{ "sub", SUB },
	{ "test", TEST },
	{ "wait", WAIT },
	{ "xchg", XCHG },
	{ "xlat", XLAT },
	{ "xor", XOR },
};



std::string translate_function(
	TMessage* tmsg
);

std::string translate_block(
	TMessage* tmsg
);

std::vector<std::string> get_function_names(
	char* buffer,
	size_t buff_size
);

std::vector<Function> get_all_go_functions(
	char* buffer,
	size_t buff_size
);

std::vector<std::string> get_wrapped_strings_from_function(
	Function* function,
	char* buffer,
	std::size_t buffer_size,
	std::uintptr_t rdata_start,
	std::uintptr_t wrapped_delta
);

std::vector<std::string> get_strings_from_function(
	Function* function,
	char* buffer,
	std::uintptr_t delta
);

std::vector<std::string> get_main_defined_functions(
	const std::vector<std::string>& total_funcs
);

std::vector<std::string> get_all_imports(
	const std::vector<std::string>& func_names
);

void define_main_pkg_funcs(
	std::vector<Function>* functions,
	std::vector<std::string>* names
);

void define_other_pkg_funcs(
	std::vector<Function>* functions,
	std::vector<std::string>* names
);

std::string find_function_name(
	std::vector<Function>* functions,
	std::uintptr_t x32_addr
);

std::vector<std::string> get_all_function_labels(
	char* buffer,
	size_t buff_size,
	std::string last_func_name
);

void fix_function_labels(
	std::vector<std::string>* labels,
	std::vector<std::string>* names
);