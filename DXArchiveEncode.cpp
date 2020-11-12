// ============================================================================
//
//			DXA file creation software
//
//	Creator			: Takumi Yamada
//	Creation Date	: 2003/09/13
//	Version			: 1.08a
//
// ============================================================================

// include ----------------------------
#include "DXArchive.h"
#include "FileLib.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <direct.h>

// define -----------------------------
#define VERSION			"1.08a"				// version
#define FILETYPE		"wolf"				// extension

// code -------------------------------

// メイン関数
int main( int argc, char *argv[] )
{
	char SrcPath[256], DestPath[256], *SrcDirectoryPath, *DestFilePath ;
	char *KeyString;
	bool Press ;
	bool NoOutput ;
	bool NoKey ;
	bool AlwaysHuffman ;
	int HuffmanSize ;
	int i ;

	printf( "WOLF file encoder. Ver%s\n", VERSION ) ;

	// 引数が一個だったらヘルプを出す
	if( argc == 1 ) goto ERR ;

	Press = true ;
	NoOutput = false ;
	NoKey = false ;
	HuffmanSize = 10 ;
	AlwaysHuffman = false ;
	SrcDirectoryPath = NULL ;
	DestFilePath = NULL ;
	KeyString = NULL ;
	for( i = 1 ; i < argc ; i ++ )
	{
		if( argv[i][0] == '-' && argv[i][1] == 'K' && argv[i][2] == ':' )
		{
			KeyString = argv[i] + 3 ;
		}
		else
		if( argv[i][0] == '-' && argv[i][1] == 'N' && argv[i][2] == 'P' && argv[i][3] == '\0' )
		{
			Press = false ;
		}
		else
		if( argv[i][0] == '-' && argv[i][1] == 'N' && argv[i][2] == 'S' && argv[i][3] == '\0' )
		{
			NoOutput = true ;
		}
		else
		if( argv[i][0] == '-' && argv[i][1] == 'N' && argv[i][2] == 'K' && argv[i][3] == '\0' )
		{
			NoKey = true ;
		}
		else
		if( argv[i][0] == '-' && argv[i][1] == 'H' && argv[i][2] == ':' )
		{
			HuffmanSize = atoi( argv[i] + 3 ) ;
			if( HuffmanSize < 0 )
			{
				HuffmanSize = 0 ;
			}
			else
			if( HuffmanSize > 255 )
			{
				HuffmanSize = 255 ;
			}
		}
		else
		if( argv[i][0] == '-' && argv[i][1] == 'A' && argv[i][2] == 'H' && argv[i][3] == '\0' )
		{
			AlwaysHuffman = true ;
		}
		else
		if( SrcDirectoryPath == NULL )
		{
			SrcDirectoryPath = argv[i] ;
		}
		else
		{
			DestFilePath = argv[i] ;
		}
	}

	if( SrcDirectoryPath == NULL )
		goto ERR ;

	// ソースパスのフルパスを作成する
	ConvertFullPath__( SrcDirectoryPath, SrcPath ) ;

	// 出力ファイルパスの作成
	{
		if( DestFilePath != NULL )
		{
			char temp[256] ;

			strcpy( temp, DestFilePath ) ;
			ConvertFullPath__( temp, DestPath ) ;
		}
		else
		{
			// ソースパスから出力ファイル名を作成する
			sprintf( DestPath, "%s.%s", SrcPath, FILETYPE ) ;
		}
	}

	// アーカイブファイルを作成する
	DXArchive::EncodeArchiveOneDirectory( DestPath, SrcPath, Press, AlwaysHuffman, ( u8 )HuffmanSize, KeyString, NoKey, NoOutput ? false : true ) ;

	// 終了
	return 0 ;

ERR:

	printf( "DxaEncode.exe [-K:KeyString] [-OPTION] SrcDirectoryPath [DestFilePath]\n\n" ) ;
	printf( " * Create archive by dragging and dropping the folder you want to encode\n" ) ;
	printf( " * -K Set encryption key (maximum 12 characters) Example: -K:KeyCode\n" ) ;
	printf( " * -NP Do not compress when archiving\n" ) ;
	printf( " * -NS not to display the progress status\n" ) ;
	printf( " * -K no encryption" ) ;
	printf( " * -H Size of the file's end in KB for Huffman compression (0 to 255)\n" ) ;
	printf( "    The default is 10, e.g. -H:20 (with -H:255 the whole file is compressed with Huffman compression)\n" ) ;
	printf( " * -AH All files are subjected to Huffman compression\n" ) ;
	printf( "    (By default, only major file formats such as png, wav, etc.)\n" ) ;
	return -1; 
}


