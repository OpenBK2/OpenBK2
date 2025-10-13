#pragma once

#include <cstdint>

class CMapEditorSingletonBase
{
protected:
  bool SendCommand( HWND hWndDst, HWND hWndSrc, uint32_t dwCommand, uint32_t dwDataLength, const void* pData ) const;

  static string MAP_FILE_NAME;
  static const uint32_t MAP_FILE_MAX_SIZE;
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

