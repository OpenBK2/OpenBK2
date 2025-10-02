#pragma once

#include "InterfaceScreenBase.h"
#include "UIElementsHelper.h"


struct SHallOfFameEntry
{
	ZDATA
	int nScore;
	wstring wszName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nScore); f.Add(3,&wszName); return 0; }
};

class CHallOfFame
{
	ZDATA
	vector<SHallOfFameEntry> entries;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&entries); return 0; }
public:
	CHallOfFame();
	void AddRecord( const wstring &wszName, int nScore )
	{
		SHallOfFameEntry val;
		val.wszName = wszName;
		val.nScore = nScore;
		entries.push_back( val );
		// write file
		CFileStream stream( "HallOfFame.bin", CFileStream::WIN_CREATE );
		if ( !stream.IsOk() )
			return;

		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		operator&( *pSaver.GetPtr() );
	}
	int GetNRecords() const { return entries.size(); }
	const SHallOfFameEntry & GetEntry( int i ) const { return  entries[i]; }
};

struct SHallOfFameEntryCompare
{
	bool operator()( const SHallOfFameEntry &s1, const SHallOfFameEntry &s2 ) const
		{	return s1.nScore > s2.nScore; }
};

class CInterfaceHallOfFame : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceHallOfFame );
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;	
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); return 0; }
	void MakeInterior();

public:
	CInterfaceHallOfFame();

	bool Init();
	bool StepLocal( bool bAppActive );
	void OnGetFocus( bool bFocus );
	
	void MsgBack( const SGameMessage &msg );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const { return 0; }
	//}
};

class CICHallOfFame : public CInterfaceCommandBase<CInterfaceHallOfFame>
{
	OBJECT_BASIC_METHODS( CICHallOfFame );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


