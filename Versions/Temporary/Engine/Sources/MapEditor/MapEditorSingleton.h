#pragma once


class CMapEditorSingletonBase
{
protected:
  bool SendCommand( HWND hWndDst, HWND hWndSrc, DWORD dwCommand, DWORD dwDataLength, const void* pData ) const;

  static string MAP_FILE_NAME;
  static const DWORD MAP_FILE_MAX_SIZE;
public:
	enum ECommandType
  {
    OPEN_FILE = 0x10,
  };

	static void SetMapFileName( const string &szMapFileName );
};

class CMapEditorSingletonApp : public CMapEditorSingletonBase
{
private:
  HANDLE hMapFileHandle;
  void *pMapFileData;

public:
  CMapEditorSingletonApp();
  ~CMapEditorSingletonApp();
  
	bool CreateMapFile( HWND hWndApp );
  void RemoveMapFile();
};


class CMapEditorSingletonChecker : public CMapEditorSingletonBase
{
private:
  HWND GetAppHwnd() const;

public:
  bool BringAppOnTop() const;
	bool OpenFileOnApp( const string &rszFilePath ) const;
};

