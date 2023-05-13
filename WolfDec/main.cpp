#include <DXArchive.h>
#include <DXArchiveVer5.h>
#include <DXArchiveVer6.h>
#include <FileLib.h>
#include <argagg.hpp>
#include "atlstr.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

////////////////////////
// WolfDec Version 0.3.3
////////////////////////

std::basic_ostream <TCHAR>& tcout =
#ifdef _UNICODE
std::wcout;
#else
std::cout;
#endif // _UNICODE

using DecryptFunction = int (*)(TCHAR*, const TCHAR*, const char*);

struct DecryptMode
{
	DecryptMode(const std::wstring& name, const DecryptFunction& decFunc, const std::vector<char> key) :
		name(name),
		decFunc(decFunc),
		key(key)
	{
	}

	DecryptMode(const std::wstring& name, const DecryptFunction& decFunc, const std::string& key) :
		name(name),
		decFunc(decFunc),
		key(std::begin(key), std::end(key))
	{
		this->key.push_back(0x00); // The key needs to end with 0x00 so the parser knows when to stop
	}


	DecryptMode(const std::wstring& name, const DecryptFunction& decFunc, const std::vector<unsigned char> key) :
		name(name),
		decFunc(decFunc)
	{
		std::copy(std::begin(key), std::end(key), std::back_inserter(this->key));
	}

	std::wstring name;
	DecryptFunction decFunc;
	std::vector<char> key;
};

const std::vector<DecryptMode> DECRYPT_MODES = {
	{L"Wolf RPG v2.01", &DXArchive_VER5::DecodeArchive, std::vector<unsigned char>{ 0x0f, 0x53, 0xe1, 0x3e, 0x04, 0x37, 0x12, 0x17, 0x60, 0x0f, 0x53, 0xe1 } },
	{L"Wolf RPG v2.10", &DXArchive_VER5::DecodeArchive, std::vector<unsigned char>{ 0x4c, 0xd9, 0x2a, 0xb7, 0x28, 0x9b, 0xac, 0x07, 0x3e, 0x77, 0xec, 0x4c } },
	{L"Wolf RPG v2.20", &DXArchive_VER6::DecodeArchive, std::vector<unsigned char>{ 0x38, 0x50, 0x40, 0x28, 0x72, 0x4f, 0x21, 0x70, 0x3b, 0x73, 0x35, 0x38 } },
	{L"Wolf RPG v2.281", &DXArchive::DecodeArchive, "WLFRPrO!p(;s5((8P@((UFWlu$#5(=" },
	{L"Wolf RPG v3.10", &DXArchive::DecodeArchive, std::vector<unsigned char>{ 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0xB5, 0x41, 0x91, 0x52, 0x16, 0x55, 0xAE, 0x34, 0xC9, 0x8F, 0x79, 0x59, 0x2F, 0x59, 0x6B, 0x95, 0x19, 0x9B, 0x1B, 0x35, 0x9A, 0x2F, 0xDE, 0xC9, 0x7C, 0x12, 0x96, 0xC3, 0x14, 0xB5, 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0x00 } },
	{L"Wolf RPG v3.173", &DXArchive::DecodeArchive, std::vector<unsigned char>{ 0x31, 0xF9, 0x01, 0x36, 0xA3, 0xE3, 0x8D, 0x3C, 0x7B, 0xC3, 0x7D, 0x25, 0xAD, 0x63, 0x28, 0x19, 0x1B, 0xF7, 0x8E, 0x6C, 0xC4, 0xE5, 0xE2, 0x76, 0x82, 0xEA, 0x4F, 0xED, 0x61, 0xDA, 0xE0, 0x44, 0x5B, 0xB6, 0x46, 0x3B, 0x06, 0xD5, 0xCE, 0xB6, 0x78, 0x58, 0xD0, 0x7C, 0x82, 0x00 } },
	{L"One Way Heroics", &DXArchive::DecodeArchive, "nGui9('&1=@3#a" },
	{L"One Way Heroics Plus", &DXArchive::DecodeArchive, "Ph=X3^]o2A(,1=@3#a" }
};

uint32_t g_mode = -1;

bool unpackArchive(const uint32_t mode, const TCHAR* pFilePath)
{
	TCHAR fullPath[MAX_PATH];
	TCHAR filePath[MAX_PATH];
	TCHAR directoryPath[MAX_PATH];
	TCHAR fileName[MAX_PATH];
	bool failed = true;

	if (mode >= DECRYPT_MODES.size())
	{
		std::wcerr << "Specified Mode: " << mode << " out of range" << std::endl;
		return false;
	}

	const DecryptMode curMode = DECRYPT_MODES.at(mode);

	ConvertFullPath__(pFilePath, fullPath);

	AnalysisFileNameAndDirPath(fullPath, filePath, directoryPath);

	SetCurrentDirectory(directoryPath);

	AnalysisFileNameAndExeName(filePath, fileName, NULL);

	CreateDirectory(fileName, NULL);

	SetCurrentDirectory(fileName);

	failed = curMode.decFunc(fullPath, TEXT(""), curMode.key.data()) < 0;
	if (failed) {
		SetCurrentDirectory(directoryPath);
		RemoveDirectory(fileName);
	}
	else {
		std::wcout << " Mode: " << wstring(curMode.name) << " ";
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

void detectMode(const TCHAR* pProgName, const TCHAR* pFilePath)
{
	tcout << TEXT("Unpacking: ") << pFilePath << TEXT("... ");

	bool success = false;

	if (g_mode == -1)
	{
		for (uint32_t i = 0; i < DECRYPT_MODES.size(); i++)
		{
			success = runProcess(pProgName, pFilePath, i);
			if (success)
			{
				g_mode = i;
				break;
			}
		}
	}
	else
		success = runProcess(pProgName, pFilePath, g_mode);

	tcout << (success ? TEXT("OK") : TEXT("FAIL")) << std::endl;
}

std::wstring s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

int wmain(int argc, TCHAR* argv[])
{
	int ret = 0;

	argagg::parser argparser {{
		{ L"help", {L"-h", L"--help"}, L"Shows this help message", 0}
		,{ L"mode", {L"-m", L"--mode"}, L"Mode index", 1}
	}};

	argagg::parser_results args;
	try {
		args = argparser.parse(argc, argv);
	} catch (const std::exception& e) {
		std::wcout << e.what() << '\n';
		return EXIT_FAILURE;
	}

	if (args[L"help"] || args.pos.size() == 0) {
		argagg::fmt_ostream fmt(std::wcout);
		fmt << "Usage: " << argv[0] << " [-m N] <A.wolf B.wolf...|mask>" << std::endl;
		fmt << argparser;
		fmt << "    Modes:" << std::endl;
		for (uint32_t i = 0; i < DECRYPT_MODES.size(); i++)
			fmt << "     " << i << ": " << DECRYPT_MODES[i].name << std::endl;
		return EXIT_SUCCESS;
	}
	else if (args[L"mode"])
	{
		uint32_t mode = args[L"mode"];
		ret = unpackArchive(mode, args.pos[0]);
	}
	else
	{
		if (args.pos.size() == 1) {
			HANDLE hFind;
			WIN32_FIND_DATAW FindFileData;

			if ((hFind = FindFirstFile(args.pos[0], &FindFileData)) != INVALID_HANDLE_VALUE)
			{
				do {
					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
						|| wcscmp(FindFileData.cFileName, L".") == 0
						|| wcscmp(FindFileData.cFileName, L"..") == 0) continue;

					detectMode(args.program, FindFileData.cFileName);
				} while (FindNextFile(hFind, &FindFileData));
				FindClose(hFind);
			}
		}
		else
			for (int i = 1; i < args.pos.size(); i++)
				detectMode(args.program, args.pos[i]);
	}

	return ret;
}
