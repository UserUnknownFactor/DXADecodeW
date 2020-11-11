// ============================================================================
//
//			ＤＸＡファイルデコードソフト
//
//	Creator			: 山田　巧 et al
//	Creation Date	: 2003/09/13
//	Version			: 1.06a
//
// ============================================================================

// include ----------------------------
#include "DXArchive.h"
#include "DXArchiveVer5.h"
#include "FileLib.h"
#include <windows.h>

// define -----------------------------
#define VERSION			"1.06a"				// バージョン
#define FILETYPE		"dxa"				// 拡張子

// data type --------------------------

// data -------------------------------

// function proto type ----------------

// function code ----------------------

int main( int argc, char *argv[] )
{
	char FullPath[MAX_PATH], FilePath[MAX_PATH], DirectoryPath[MAX_PATH], FileName[MAX_PATH] ;
	char *DxaFilePath, *KeyString ;

	printf( "WOLF file decoder. Ver%s\n\n", VERSION ) ;

	if( argc == 1 ) goto ERR ;

	if( strnicmp( argv[1], "-G", 3 ) == 0 )
	{
		DxaFilePath = argv[2] ;
		printf( "Guessing key from %s...\n", DxaFilePath ) ;

		unsigned char *Key = (unsigned char *)malloc(13);
		unsigned char *KeyRead = Key;

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile(DxaFilePath, &FindFileData );
		FILE * pFile = fopen( FindFileData.cFileName, "rb" ) ;

		fseeko64(pFile, 0xC, SEEK_SET ) ;
		fread(KeyRead, 1, 4, pFile) ;
		KeyRead +=4;

		fseeko64(pFile, 0x1C, SEEK_SET ) ;
		fread(KeyRead, 1, 4, pFile) ;
		KeyRead +=4;

		fseeko64(pFile, 0x14, SEEK_SET ) ;
		fread(KeyRead, 1, 4, pFile) ;
		KeyRead +=4;

		Key[13] = 0;

		DXArchive::printKey(Key);

		fclose( pFile ) ;
		free( Key ) ;
		FindClose( hFind ) ;

		return 0 ;
	}

	if( strnicmp( argv[1], "-K:", 3 ) == 0 )
	{
		if( argc == 2 ) goto ERR ;

		KeyString   = argv[1] + 3 ;
		DxaFilePath = argv[2] ;
	}
	else if( strnicmp( argv[1], "-NK", 3 ) == 0 )
	{
		KeyString   = NULL ;
		DxaFilePath = argv[2] ;
	}
	else
	{
		KeyString   = (char *)"8P@(rO!p;s5" ;
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

			if( DXArchive::DecodeArchive( tmpFullPath, (char *)"", KeyString ) < 0 )
			{
				if (DXArchive_VER5::DecodeArchive( tmpFullPath, (char *)"", KeyString ) < 0)
				{
					SetCurrentDirectory( DirectoryPath ) ;
					RemoveDirectory( FileName );
					printf( "FAILED\n") ;
				}
				else {
					printf( "COMPLETE\n") ;
				}
			} else
			{
				printf( "COMPLETE\n") ;
			}

			SetCurrentDirectory( DirectoryPath ) ;

		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	// 終了
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
