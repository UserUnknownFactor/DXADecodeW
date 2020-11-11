// ============================================================================
//
//			ＤＸＡファイル作成ソフト
//
//	Creator			: 山田　巧 et al
//	Creation Date	: 2003/09/13
//	Version			: 1.06a
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
#define VERSION			"1.06a"				// バージョン
#define FILETYPE		"dxa"				// 拡張子

// code -------------------------------

// メイン関数
int main( int argc, char *argv[] )
{
	char SrcPath[256], DestPath[256], *KeyString, *SrcDirectoryPath, *DestFilePath ;
	bool Press ;
	int i ;

	printf( "WOLF file encoder. Ver%s\n", VERSION ) ;

	// 引数が一個だったらヘルプを出す
	if( argc == 1 ) goto ERR ;

	Press = true ;
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
	DXArchive::EncodeArchiveOneDirectory( DestPath, SrcPath, Press, KeyString ) ;

	// 終了
	return 0 ;

ERR:

	printf( "DxaEncode.exe [-K:KeyString] [-NP] SrcDirectoryPath [DestFilePath]\n\n" ) ;
	printf( " * Create archive by dragging and dropping the folder you want to encode\n" ) ;
	printf( " * -K Set encryption key (maximum 12 characters) Example: -K:KeyCode\n" ) ;
	printf( " * -NP Do not compress when archiving\n" ) ;
	return -1;
}


