// ============================================================================
//
//			DXA file creation software
//
//
//	Creator			: Takumi Yamada
//	Creation Date	: 2003/09/13
//	Version			: 1.08a
//
// ============================================================================

// include ----------------------------
#include "DXArchive.h"
#include "DXArchiveVer5.h"
#include "FileLib.h"
#include <windows.h>

// define -----------------------------
#define VERSION			"1.08a"				// version
#define FILETYPE		"wolf"				// extension

// data type --------------------------

// data -------------------------------

const unsigned char gKEY_2_10[] = {0x0f, 0x53, 0xe1, 0x3e, 0x04, 0x37, 0x12, 0x17, 0x60, 0x0f, 0x53, 0xe1};
const unsigned char gKEY_2_10_2[] = {0x4c, 0xd9, 0x2a, 0xb7, 0x28, 0x9b, 0xac, 0x07, 0x3e, 0x77, 0xec, 0x4c};
const unsigned char gKEY_2_20[] = {0x38, 0x50, 0x40, 0x28, 0x72, 0x4f, 0x21, 0x70, 0x3b, 0x73, 0x35, 0x38};

// function proto type ----------------

// function code ----------------------

// メイン関数
int main( int argc, char *argv[ ] )
{
	char FullPath[MAX_PATH], FilePath[MAX_PATH], DirectoryPath[MAX_PATH], FileName[MAX_PATH] ;
	char *DxaFilePath ;
	char *KeyString ;

	printf( "WOLF file decoder. Ver%s\n\n", VERSION ) ;

	if( argc == 1 ) goto ERR ;

	if(_strnicmp( argv[1], "-G", 3 ) == 0 )
	{
		DxaFilePath = argv[2] ;
		printf( "Guessing key from %s...\n", DxaFilePath ) ;

		unsigned char *Key = (unsigned char *)malloc(14);
		unsigned char *KeyRead = Key;

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile(DxaFilePath, &FindFileData );
		FILE * pFile = fopen( FindFileData.cFileName, "rb" ) ;

		fseek(pFile, 0xC, SEEK_SET ) ;
		if (auto size = fread(KeyRead, 1, 4, pFile) > 11) {
			KeyRead += 4;

			fseek(pFile, 0x1C, SEEK_SET);
			fread(KeyRead, 1, 4, pFile);
			KeyRead += 4;

			fseek(pFile, 0x14, SEEK_SET);
			fread(KeyRead, 1, 4, pFile);
			KeyRead += 4;

			Key[13] = 0;

			DXArchive::printKey(Key);

			fclose(pFile);
			free(Key);
			FindClose(hFind);
		}


		return 0 ;
	}

	if(_strnicmp( argv[1], "-K:", 3 ) == 0 )
	{
		if( argc == 2 ) goto ERR ;

		KeyString   = argv[1] + 3 ;
		DxaFilePath = argv[2] ;
	}
	else if(_strnicmp( argv[1], "-NK", 3 ) == 0 )
	{
		KeyString   = NULL ;
		DxaFilePath = argv[2] ;
	}
	else
	{
		KeyString   = NULL ;
		DxaFilePath = argv[1] ;
	}

	// Get full path of DXA file
	ConvertFullPath__( DxaFilePath, FullPath ) ;

	// Get separate directory and filename
	AnalysisFileNameAndDirPath( FullPath, FilePath, DirectoryPath ) ;

	SetCurrentDirectory( DirectoryPath ) ;

	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	if ((hFind = FindFirstFile(DxaFilePath, &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do {
			if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
				|| strcmp(FindFileData.cFileName, ".") == 0
				|| strcmp(FindFileData.cFileName, "..") == 0) continue;

			// Get file name with extension removed
			AnalysisFileNameAndExeName( FindFileData.cFileName, FileName, NULL ) ;

			CreateDirectory( FileName , NULL ) ;
			SetCurrentDirectory( FileName ) ;

			char tmpFullPath[MAX_PATH] = "";
			strcat(tmpFullPath, DirectoryPath);
			strcat(tmpFullPath, (char *)"\\");
			strcat(tmpFullPath, FindFileData.cFileName);

			printf( "Decoding %s... ", FindFileData.cFileName ) ;
			bool failed = false;

			if (KeyString == NULL) {
				failed = DXArchive_VER5::DecodeArchive(tmpFullPath, (char *)"", (const char*)gKEY_2_10) < 0;
				if (failed) failed = DXArchive::DecodeArchive(tmpFullPath, (char *)"", (const char*)gKEY_2_20) < 0;
				if (failed) failed = DXArchive::DecodeArchive(tmpFullPath, (char *)"", (const char*)NULL) < 0;
				if (failed) failed = DXArchive_VER5::DecodeArchive(tmpFullPath, (char *)"", (const char*)gKEY_2_10_2) < 0;
				if (failed) failed = DXArchive_VER5::DecodeArchive(tmpFullPath, (char *)"", (const char*)NULL) < 0;
			}
			else {
				failed = DXArchive::DecodeArchive( tmpFullPath, (char *)"", KeyString ) < 0;
				if (failed) failed = DXArchive_VER5::DecodeArchive( tmpFullPath, (char *)"", KeyString ) < 0;
			}
			
			if(failed) {
				SetCurrentDirectory(DirectoryPath);
				RemoveDirectory(FileName);
			}

			printf("%s\n", (failed ? "FAILED" : "COMPLETE"));
			SetCurrentDirectory( DirectoryPath ) ;

		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return 0 ;

ERR:
	printf("DxaDecode.exe [-K:KeyString] [-G] [-NK] WolfArchiveFilePath\n\n") ;
	printf(" * Unpack by dragging and dropping DXA file\n");
	printf(" * If specific encryption key is used you need to specify it with -K option\n");
	printf(" * Use -NK for system default key\n");
	printf(" * Use -G to guess the key\n\n");
	printf(" Examples:\n   DxaDecode -K:55AA2055550655AA55D57C66 \"*.wolf\"\n   DxaDecode -G Demo.wolf\n");
	return -1 ;
}

	

