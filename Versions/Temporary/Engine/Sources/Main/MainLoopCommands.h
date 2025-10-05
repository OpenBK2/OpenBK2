#pragma once

#include "Main_export.h"

#include "MainLoop.h"

MAIN_EXPORT IInterfaceCommand *CreateICExitGame();
IInterfaceCommand *CreateICLoad( const std::string &szName );
MAIN_EXPORT IInterfaceCommand *CreateICSave( const std::string &szName );
MAIN_EXPORT IInterfaceCommand *CreateICCloseInterface();

MAIN_EXPORT IBinSaver *CreateSaveLoadSaver( CDataStream *pStream, ESaverMode mode );
IBinSaver *CreateSaveSaverWithCheckers( CDataStream *pStream, std::vector< CPtr<IDebugSaveCheckObj> > &checkers );

class MAIN_EXPORT CICLoadBase : public IInterfaceCommand
{
	std::wstring szTitleName;
	std::string szFileName;
public:
	enum EStage
	{
		STG_START,
		STG_SERIALIZE_DONE,
		STG_AFTER_LOAD_DONE,
	};
	
	CICLoadBase();
	CICLoadBase( const std::string &szFileName );
	
	void Configure( const char *pszConfig );
	void Exec();
	
	virtual void OnProgress( EStage eStage ) {}

	int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Wrong call" ); return 0; }
};

class MAIN_EXPORT CICSaveBase : public IInterfaceCommand
{
	ZDATA
	std::wstring szTitleName;
	std::string szFileName;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szTitleName); f.Add(3,&szFileName); return 0; }
protected:
	const std::string &GetFileName() const { return szFileName; }
	const std::string GetPathName();
public:
	enum EStage
	{
		STG_START,
		STG_AFTER_SAVE_DONE,
	};

	CICSaveBase();
	CICSaveBase( const std::string &szFileName );
	
	void Configure( const char *pszConfig );
	void Exec();

	virtual void OnProgress( EStage eStage ) {}
};


