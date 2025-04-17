#include <DXArchive.h>
#include <DXArchiveVer5.h>
#include <DXArchiveVer6.h>
#include <FileLib.h>

#include "atlstr.h"

#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#include <argagg.hpp>

////////////////////////
// WolfDec Version 0.3.3
////////////////////////

std::basic_ostream <TCHAR>& INFO_LOG =
#ifdef _UNICODE
std::wcout;
#else
std::cout;
#endif // _UNICODE

using DecryptFunction = int (*)(TCHAR*, const TCHAR*, const char*);
using EncryptFunction = int (*)(const TCHAR*, const TCHAR*, bool, const char*, uint16_t);

class InvalidModeException : public std::exception{};

struct CryptMode
{
	CryptMode(const std::wstring& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::vector<char> key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc),
		key(key)
	{
	}

	CryptMode(const std::wstring& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::string& key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc),
		key(key.begin(), key.end())
	{
		this->key.push_back(0x00); // The key needs to end with 0x00 so the parser knows when to stop
	}

	CryptMode(const std::wstring& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::vector<unsigned char> key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc)
	{
		std::copy(key.begin(), key.end(), std::back_inserter(this->key));
	}

	std::wstring name;
	uint16_t cryptVersion;
	DecryptFunction decFunc;
	EncryptFunction encFunc;
	std::vector<char> key;
};

static constexpr uint16_t PRO_CRYPT_VERSION = 1000;
static constexpr uint16_t CC2_PRO_VERSION = 0xC8;

using CryptModes = std::vector<CryptMode>;

static CryptModes DEFAULT_CRYPT_MODES = {
	{ L"Wolf RPG v2.01", 0x0, &DXArchive_VER5::DecodeArchive, &DXArchive_VER5::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x0f, 0x53, 0xe1, 0x3e, 0x04, 0x37, 0x12, 0x17, 0x60, 0x0f, 0x53, 0xe1 } },
	{ L"Wolf RPG v2.10", 0x0, &DXArchive_VER5::DecodeArchive, &DXArchive_VER5::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x4c, 0xd9, 0x2a, 0xb7, 0x28, 0x9b, 0xac, 0x07, 0x3e, 0x77, 0xec, 0x4c } },
	{ L"Wolf RPG v2.20", 0x0, &DXArchive_VER6::DecodeArchive, &DXArchive_VER6::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x38, 0x50, 0x40, 0x28, 0x72, 0x4f, 0x21, 0x70, 0x3b, 0x73, 0x35, 0x38 } },
	{ L"Wolf RPG v2.225", 0x0, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, "WLFRPrO!p(;s5((8P@((UFWlu$#5(=" },
	{ L"Wolf RPG v3.00", 0x12C, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0xB5, 0x41, 0x91, 0x52, 0x16, 0x55, 0xAE, 0x34, 0xC9, 0x8F, 0x79, 0x59, 0x2F, 0x59, 0x6B, 0x95, 0x19, 0x9B, 0x1B, 0x35, 0x9A, 0x2F, 0xDE, 0xC9, 0x7C, 0x12, 0x96, 0xC3, 0x14, 0xB5, 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0x00 } },
	{ L"Wolf RPG v3.14", 0x13A, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0x31, 0xF9, 0x01, 0x36, 0xA3, 0xE3, 0x8D, 0x3C, 0x7B, 0xC3, 0x7D, 0x25, 0xAD, 0x63, 0x28, 0x19, 0x1B, 0xF7, 0x8E, 0x6C, 0xC4, 0xE5, 0xE2, 0x76, 0x82, 0xEA, 0x4F, 0xED, 0x61, 0xDA, 0xE0, 0x44, 0x5B, 0xB6, 0x46, 0x3B, 0x06, 0xD5, 0xCE, 0xB6, 0x78, 0x58, 0xD0, 0x7C, 0x82, 0x00 } },
	{ L"Wolf RPG v3.31", 0x14B, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xCA, 0x08, 0x4C, 0x5D, 0x17, 0x0D, 0xDA, 0xA1, 0xD7, 0x27, 0xC8, 0x41, 0x54, 0x38, 0x82, 0x32, 0x54, 0xB7, 0xF9, 0x46, 0x8E, 0x13, 0x6B, 0xCA, 0xD0, 0x5C, 0x95, 0x95, 0xE2, 0xDC, 0x03, 0x53, 0x60, 0x9B, 0x4A, 0x38, 0x17, 0xF3, 0x69, 0x59, 0xA4, 0xC7, 0x9A, 0x43, 0x63, 0xE6, 0x54, 0xAF, 0xDB, 0xBB, 0x43, 0x58, 0x00 } },
	{ L"Wolf RPG v3.50", 0x15E, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xD2, 0x84, 0xCE, 0x28, 0xCE, 0x88, 0x82, 0xE4, 0x2A, 0x18, 0x2E, 0x4C, 0x06, 0xB4, 0xEA, 0x84, 0x06, 0xB8, 0xC6, 0x88, 0x5A, 0xA0, 0x9E, 0x7C, 0x56, 0x40, 0xBA, 0x34, 0x52, 0xCC, 0xC6, 0x7C, 0x2E, 0x14, 0x12, 0x68, 0xFE, 0x5C, 0x76, 0x94, 0x86, 0x78, 0x8E, 0x4C, 0xBE, 0x88, 0x66, 0x9C, 0x1E, 0xE0, 0x8E, 0x6C, 0x00 } },
	{ L"Wolf RPG ChaCha2 v1", 0x64, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xC9, 0x82, 0xF8, 0xB4, 0x2C, 0x93, 0x9E, 0x83, 0x0E, 0xBC, 0xBC, 0x92, 0x68, 0x8D, 0x59, 0xA1, 0x4A, 0x9E, 0x7F, 0xB0, 0xAC, 0xAF, 0x1D, 0x8F, 0x8E, 0xB8, 0x3B, 0x9E, 0xE8, 0x89, 0xD9, 0xAD, 0xFF, 0xBC, 0x2D, 0xAB, 0x9D, 0x8B, 0x0F, 0xB4, 0xBB, 0x9A, 0x69, 0x85, 0x00 } }, // First 32 bytes of the key and the next 12 byte are the nonce, 0 terminator for the unused keygen to not crash
	{ L"Custom Key (v2.281+)", 0, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, "" }, // 8
};

uint32_t g_mode = -1;

int unpackArchive(const TCHAR* pFilePath, const uint32_t mode)
{
	TCHAR fullPath[MAX_PATH];
	TCHAR filePath[MAX_PATH];
	TCHAR directoryPath[MAX_PATH];
	TCHAR fileName[MAX_PATH];
	int failed = 1;

	if (mode < 0 || mode >= DEFAULT_CRYPT_MODES.size())
	{
		std::wcerr << "Specified mode: " << mode << " is unknown or not found" << std::endl;
		return failed;
	}

	const CryptMode curMode = DEFAULT_CRYPT_MODES.at(mode);

	ConvertFullPath__(pFilePath, fullPath);

	AnalysisFileNameAndDirPath(fullPath, filePath, directoryPath);

	SetCurrentDirectory(directoryPath);

	AnalysisFileNameAndExeName(filePath, fileName, NULL);

	CreateDirectory(fileName, NULL);

	SetCurrentDirectory(fileName);
 
	try {
		failed = curMode.decFunc(fullPath, L"", curMode.key.data()) < 0;
	} 
	catch(...) {}

	if (failed) {
		//std::wcout << "\n Failed to decode: " << wstring(fileName) << " ";
		SetCurrentDirectory(directoryPath);
		RemoveDirectory(fileName);
	}
	else {
		std::wcout << L"Mode: " << std::wstring(curMode.name) << " ";
	}

	return failed;
}


bool runProcess(const TCHAR* pProgName, const TCHAR* pFilePath, const uint32_t mode)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::wstring wstr = std::wstring(pProgName) + L" -m " + std::to_wstring(mode) + L" \"" + std::wstring(pFilePath) + L"\"";

	if (!CreateProcess(NULL, const_cast<LPWSTR>(wstr.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		std::wcerr << "CreateProcess failed (" << GetLastError() << ")." << std::endl;
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	bool success = false;

	DWORD exit_code;
	if (FALSE == GetExitCodeProcess(pi.hProcess, &exit_code))
		std::wcerr << "GetExitCodeProcess() failure: " << GetLastError() << std::endl;
	else
		success = (exit_code == 0);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return success;
}

uint16_t getCryptVersion(const TCHAR* pFilePath)
{
	// Read the DARC_HEAD from the file
	std::ifstream f(pFilePath, std::ios::binary);
	if (!f.is_open())
	{
		INFO_LOG << std::format(TEXT("Failed to open file: {}"), pFilePath) << std::endl;
		return 0;
	}

	DARC_HEAD header;
	f.read(reinterpret_cast<char*>(&header), sizeof(DARC_HEAD));
	f.close();

	if ((u16)header.Head != DXA_HEAD)
		return 0;

	return (header.Flags >> 16);
}


int detectModeAndUnpack(const TCHAR* pProgName, const TCHAR* pFilePath, bool final=false)
{
	INFO_LOG << L"Unpacking: " << pFilePath << L"... ";

	uint16_t cryptVersion = 0;
	bool success = false;

	if (g_mode == -1) {
		cryptVersion = getCryptVersion(pFilePath);
		if (cryptVersion != 0)
		{
			// for 3rd: use the exact version
			for (uint32_t i = 0; i < DEFAULT_CRYPT_MODES.size(); i++)
				if (DEFAULT_CRYPT_MODES.at(i).cryptVersion == cryptVersion) {
					g_mode = i;
					break;
				}
		}
		else {
			// for 2nd: check all possible 2.XX versions
			for (uint32_t i = 0; i < DEFAULT_CRYPT_MODES.size(); i++) {
				if (DEFAULT_CRYPT_MODES[i].cryptVersion > 0) continue;
				success = final ? !unpackArchive(pFilePath, i) : runProcess(pProgName, pFilePath, i);
				if (success) break;
			}
		}
	}

	if (g_mode != -1 && !success)
		success = final ? !unpackArchive(pFilePath, g_mode) : runProcess(pProgName, pFilePath, g_mode);

	INFO_LOG << (success ? L"OK" : L"FAIL") << std::endl;
	return success;
}

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo( size_needed, 0 );
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

bool valid_hex(const char* input, int len)
{
	for (int i = 0; i < len; i++) {
		char current = input[i];
		if (current >= '0' && current <= '9') continue;
		if (current >= 'A' && current <= 'F') continue;
		if (current >= 'a' && current <= 'f') continue;
		return false;
	}
	return true;
}

int char2int(char input)
{
	if(input >= '0' && input <= '9') return input - '0';
	if(input >= 'A' && input <= 'F') return input - 'A' + 10;
	if(input >= 'a' && input <= 'f') return input - 'a' + 10;
	return 0;
}

std::vector<char> parse_key(std::wstring key, bool hex_mode) {
	const wchar_t *input = key.c_str();

	size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
	char *buffer = new char[size];
	uint16_t len = (uint16_t)std::wcstombs(buffer, input, size);

	if (hex_mode && valid_hex(buffer, len)) {
		char *src = buffer;
		uint16_t i = 0;
		std::vector<char> vkey;
		vkey.reserve(len/2);
		while (src[0] && src[1] && i < len)
		{
			vkey[i++] = (unsigned char)(char2int(src[0]) << 4 | char2int(src[1]));
			src += 2;
		}
		delete[] buffer;
		return vkey;
	} else {
		std::vector<char> vkey(buffer, buffer + len);
		delete[] buffer;
		return vkey;
	}
}

void showHelp(TCHAR* programName, const argagg::parser& argparser) {
	argagg::fmt_ostream fmt(std::wcout);
	fmt << "Usage: " << programName << " [-m num] [-s strkey|-k hexkey] [-g] <A.wolf B.wolf...|mask>" << std::endl;
	fmt << argparser;
	fmt << "	Modes:" << std::endl;
	for (uint32_t i = 0; i < DEFAULT_CRYPT_MODES.size(); i++)
		fmt << "	 " << i << ": " << DEFAULT_CRYPT_MODES[i].name << std::endl;
}

uint32_t setupMode(const argagg::parser_results& args) {
	size_t modes_size = DEFAULT_CRYPT_MODES.size() - 1;
	uint32_t mode = args[L"mode"].as<uint32_t>();

	if (mode < 0 || mode > modes_size) {
		INFO_LOG << "Unsupported mode: " << mode << std::endl;
		return -1;
	}

	if (args[L"key"] && mode == modes_size) {
		DEFAULT_CRYPT_MODES[modes_size].key = parse_key(args[L"key"].as<wstring>().c_str(), false);
	}
	else if (args[L"hex"] && mode == modes_size) {
		DEFAULT_CRYPT_MODES[modes_size].key = parse_key(args[L"hex"].as<wstring>().c_str(), true);
	}

	return mode;
}

int guessKey(const std::wstring& filename) {
	unsigned char* Key = new unsigned char[14];
	memset(Key, 0, 14);
	unsigned char* KeyRead = Key;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(filename.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		INFO_LOG << "No file named " << filename << std::endl;
		delete[] Key;
		return EXIT_FAILURE;
	}

	INFO_LOG << "Guessing key from " << filename << "..." << std::endl;
	FILE* pFile = _wfopen(FindFileData.cFileName, L"rb");

	fread(KeyRead, 1, 2, pFile);
	int ret = EXIT_FAILURE;
	if (hFind == NULL || strcmp((const char*)KeyRead, "DX") == 0) {
		INFO_LOG << "New WolfRPG versions don't support key guessing." << std::endl;
	}
	else {
		memset(Key, 0, 14);
		fseek(pFile, 0xC, SEEK_SET);
		if (auto size = fread(KeyRead, 1, 4, pFile) > 1) {
			KeyRead += 4;

			fseek(pFile, 0x1C, SEEK_SET);
			fread(KeyRead, 1, 4, pFile);
			KeyRead += 4;

			fseek(pFile, 0x14, SEEK_SET);
			fread(KeyRead, 1, 4, pFile);
			KeyRead += 4;

			Key[13] = 0;
			char hexstr[26];
			for (int j = 0; j < 12; j++)
				sprintf(hexstr + j * 2, "%02X", Key[j]);
			hexstr[25] = 0;
			INFO_LOG << "Key = " << hexstr;
			ret = EXIT_SUCCESS;
		}
	}
	fclose(pFile);
	delete[] Key;
	if (hFind) FindClose(hFind);
	return ret;
}

// Generic helper function to process a file or mask with a specific operation
template<typename Func>
int processFileOrMask(const TCHAR* fileOrMask, Func operation) {
	int ret = 0;
	HANDLE hFind;
	WIN32_FIND_DATAW FindFileData;

	if ((hFind = FindFirstFile(fileOrMask, &FindFileData)) != INVALID_HANDLE_VALUE) {
		do {
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
				|| wcscmp(FindFileData.cFileName, L".") == 0
				|| wcscmp(FindFileData.cFileName, L"..") == 0) continue;

			ret = operation(FindFileData.cFileName);
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return ret;
}


int wmain(int argc, TCHAR* argv[])
{
	argagg::parser argparser{ {
		{ L"help", {L"-h", L"--help"}, L"Shows this help message", 0}
		,{ L"guess", {L"-g", L"--guess"}, L"Guess old version key", 0}
		,{ L"key", {L"-s", L"--strkey"}, L"Provide decoding string key", 1}
		,{ L"hex", {L"-k", L"--hexkey"}, L"Provide decoding hexadecimal key", 1}
		,{ L"mode", {L"-m", L"--mode"}, L"Mode index (autodetected if not provided)", 1}
		,{ L"pack", {L"-p", L"--pack"}, L"Whether to pack or unpack game files", 1}
	} };

	argagg::parser_results args;
	try {
		args = argparser.parse(argc, argv);
	}
	catch (const std::exception& e) {
		std::wcout << e.what() << '\n';
		return EXIT_FAILURE;
	}

	// Show help if requested or no arguments provided
	if (args[L"help"] || args.pos.size() == 0) {
		showHelp(argv[0], argparser);
		return EXIT_SUCCESS;
	}
	else if (args[L"guess"]) {
		return guessKey(args.pos[0]);
	}
	else {
		int finalResult = 0;

		if (args[L"mode"])
			g_mode = setupMode(args);

		const TCHAR* program = args.program;
		for (const auto& file : args.pos) {
			int fileResult = 0;
			if (filesystem::is_regular_file(file))
			{
				fileResult |= detectModeAndUnpack(program, file, true);
			}
			else
			{
				fileResult |= processFileOrMask(file, [program](const TCHAR* filename) -> int {
					std::wcout << L"Processed file: " << filename << std::endl;
					return detectModeAndUnpack(program, filename);
					});
			}

			if (fileResult != 0)
				finalResult = fileResult;
		}

		return finalResult;
	}
}