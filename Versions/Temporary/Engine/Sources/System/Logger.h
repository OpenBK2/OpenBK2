#pragma once

namespace NLog
{

struct ILogDumper : public CObjectBase
{
	virtual void Dump( const std::wstring &wszString ) = 0;
};
ILogDumper *CreateFileDumper( const std::string &szFullFileName );
ILogDumper *CreateDebugDumper();
ILogDumper *CreateAssertDumper();

class CLogger
{
	const std::string szLoggerName;
	typedef std::vector<CObj<ILogDumper> > CDumpersList;
	CDumpersList dumpers;
	std::wstring wszLogBuffer;
	CLogger *pParent;
	//
	CLogger *GetParent() const { return pParent; }
	const std::string &GetLoggerLocalName() const { return szLoggerName; }
	std::string GetLoggerFullName() const;
public:
	CLogger( const std::string &_szLoggerName, CLogger *_pParent ): szLoggerName( _szLoggerName ), pParent( _pParent ) {}
	~CLogger() { Dump(); }
	//
	CLogger &operator<<( const int nVal );
	CLogger &operator<<( const long lVal );
	CLogger &operator<<( const double fVal );
	CLogger &operator<<( const bool bVal );
	CLogger &operator<<( const char cVal );
	CLogger &operator<<( const wchar_t wcVal );
	CLogger &operator<<( const char *pszText );
	CLogger &operator<<( const std::string &szText );
	CLogger &operator<<( const wchar_t *pwszText );
	CLogger &operator<<( const std::wstring &wszText );
	//
	CLogger &operator<< ( CLogger &(*Func)( CLogger &logStream ) ) { return Func( *this ); }
	CLogger &Dump();
	bool Dump( const std::string &szChildLoggerFullName, const std::wstring &wszString );
	//
	void AddDumper( ILogDumper *pDumper ) { dumpers.push_back( pDumper ); }
};

}

NLog::CLogger &endl( NLog::CLogger &logStream ) { return logStream << L"\n"; }
NLog::CLogger &dump( NLog::CLogger &logStream ) { return logStream.Dump(); }

// Отладочные сообщения. Используются для глубокой отладки приложений (программерское).
EXTERNVAR NLog::CLogger logDebug;
// Отладочные сообщения. Используются для отладки приложений (программерское).
EXTERNVAR NLog::CLogger logInfo;
// Информационнае сообщения о серьёзных событиях в игре (инициализирована подсистема, загрузился уровень и т.п.)
EXTERNVAR NLog::CLogger logNotice;
// Предупреждение о возникновении некорректной (но обрабатываемой) ситуации (object outside map)
EXTERNVAR NLog::CLogger logWarning;
// Возникла ошибка - функция не может быть выполнена правильно, но не фатально (can't load sound file "attack.wav")
EXTERNVAR NLog::CLogger logError;
// Критическая ошибка - какие-то компоненты могут совсем отключиться (Can't detect sound card. All Sounds will be disabled)
EXTERNVAR NLog::CLogger logCritical;
// Возникла невосстановимая ошибка - программа не может дальше продолжаться корректно (Allocation failed. Not enough memory)
EXTERNVAR NLog::CLogger logAlert;
// Всё упало - unhandled exception - используется из обработчика unhandled exception
EXTERNVAR NLog::CLogger logEmergency;

