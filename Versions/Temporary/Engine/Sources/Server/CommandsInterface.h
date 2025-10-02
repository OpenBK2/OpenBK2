#pragma once

struct SCommand
{
	int nCmd;
	vector<string> params;

	const string GetStr( const int nIndex ) const;
	const int GetInt( const int nIndex ) const;
};

class CCommandsBase : public CObjectBase
{
	list<SCommand> cmds;
protected:
	void PreprocessLine( string *pszLine, vector<string> *pszWords );
	void PushCommand( const SCommand &cmd );
public:	
	virtual void GetStringCommands( vector<string> *pCommands ) = 0;
	virtual bool LineEntered( const string &szLine, string *pszErr ) = 0;

	bool GetCommand( SCommand *pCmd );
};

