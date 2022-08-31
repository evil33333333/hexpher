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

};

struct IBuffer
{
	std::string raw_asm;
	Instruction instruction;
	std::vector<std::string> raw_bytes;
	std::uintptr_t address = 0xffffffffffffffff;
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
	std::uintptr_t image_base = 0xFFFFFFFF;
	std::uintptr_t wrapped_image_base = 0xFFFFFFFF;
	std::vector<uint32_t>* instructions_used = nullptr;
	int indent_level = 1;
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

inline unsigned char gofunc_bytes[] = {0x64, 0x8B, 0x0D, 0x14, 0x00, 0x00, 0x00};
inline unsigned char gofunc_bytes_x64[] = { 0x49, 0x3B, 0x66, 0x10 };
inline unsigned char gofunc_label_terminator[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
inline unsigned char gofunc_label_start[] = { 0x74, 0x00, 0x72, 0x75, 0x6E, 0x74, 0x69, 0x6D, 0x65, 0x2E, 0x65, 0x74, 0x65, 0x78, 0x74, 0x00 };
inline std::array<std::string, 75> banned_types = { 
	"type..eq.strings.Reader",
	"type..eq.strings.singleStringReplacer",
	"type..eq.encoding/json.MarshalerError",
	"type..eq.encoding/json.UnsupportedValueError",
	"type..eq.encoding/json.SyntaxError",
	"type..eq.encoding/json.reflectWithString",
	"type..eq.[2]interface {}",
	"type..eq.struct { encoding/json.ptr interface {}; encoding/json.len int }",
	"type..eq.context.valueCtx",
	"type..eq.crypto/elliptic.CurveParams",
	"type..eq.encoding/asn1.taggedEncoder",
	"type..eq.encoding/asn1.fieldParameters",
	"type..eq.encoding/asn1.tagAndLength",
	"type..eq.[2]encoding/asn1.encoder",
	"type..eq.crypto/ecdsa.PublicKey",
	"type..eq.crypto/ecdsa.PrivateKey",
	"type..eq.crypto/rc4.Cipher",
	"type..eq.crypto/sha256.digest",
	"type..eq.container/list.List",
	"type..eq.container/list.Element",
	"type..eq.vendor/golang.org/x/net/dns/dnsmessage.nestedError",
	"type..eq.vendor/golang.org/x/net/dns/dnsmessage.ResourceHeader",
	"type..eq.internal/singleflight.Result",
	"type..eq.net.policyTableEntry",
	"type..eq.net.AddrError",
	"type..eq.net.UnixAddr",
	"type..eq.net.OpError",
	"type..eq.net.DNSError",
	"type..eq.net.ParseError",
	"type..eq.net.onlyValuesCtx",
	"type..eq.net.nssCriterion",
	"type..eq.net.dialResult·1",
	"type..eq.net.result·3",
	"type..eq.net/url.Error",
	"type..eq.net/url.Userinfo",
	"type..eq.net/url.URL",
	"type..eq.crypto/x509.CertificateInvalidError",
	"type..eq.crypto/x509.UnknownAuthorityError",
	"type..eq.crypto/x509.HostnameError",
	"type..eq.crypto/x509.rfc2821Mailbox",
	"type..eq.crypto/tls.prefixNonceAEAD",
	"type..eq.crypto/tls.xorNonceAEAD",
	"type..eq.crypto/tls.RecordHeaderError",
	"type..eq.crypto/tls.atLeastReader",
	"type..eq.compress/flate.literalNode",
	"type..eq.vendor/golang.org/x/net/idna.labelError",
	"type..eq.vendor/golang.org/x/net/http2/hpack.HeaderField",
	"type..eq.vendor/golang.org/x/net/http2/hpack.pairNameValue",
	"type..eq.net/http/internal.chunkedReader",
	"type..eq.vendor/golang.org/x/net/http/httpproxy.domainMatch",
	"type..eq.vendor/golang.org/x/net/http/httpproxy.Config",
	"type..eq.net/http.connectMethodKey",
	"type..eq.net/http.httpError",
	"type..eq.net/http.http2FrameHeader",
	"type..eq.net/http.http2PriorityParam",
	"type..eq.net/http.http2Setting",
	"type..eq.net/http.http2StreamError",
	"type..eq.net/http.http2connError",
	"type..eq.net/http.http2PingFrame",
	"type..eq.net/http.http2WindowUpdateFrame",
	"type..eq.net/http.http2PriorityFrame",
	"type..eq.net/http.http2RSTStreamFrame",
	"type..eq.net/http.http2stickyErrWriter",
	"type..eq.net/http.http2GoAwayError",
	"type..eq.net/http.readTrackingBody",
	"type..eq.struct { io.Reader; io.WriterTo }",
	"type..eq.net/http.socksUsernamePassword",
	"type..eq.github.com/mattn/go-colorable.hsv",
};

const std::string tab_string = "\t";

inline std::map<std::string, Instruction> instructions_map
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
	std::uintptr_t wrapped_image_base
);

std::vector<std::string> get_strings_from_function(
	Function* function,
	char* buffer,
	std::uintptr_t image_base
);

std::vector<std::string> get_all_user_defined_functions(
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

std::vector<std::string> get_other_defined_functions(
	const std::vector<std::string>& total_funcs
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