#pragma once

struct SCommand
{
	int nCmd;
	std::vector<std::string> params;

	const std::string GetStr( const int nIndex ) const;
	const int GetInt( const int nIndex ) const;
};

class CCommandsBase : public CObjectBase
{
	std::list<SCommand> cmds;
protected:
	void PreprocessLine( std::string *pszLine, std::vector<std::string> *pszWords );
	void PushCommand( const SCommand &cmd );
public:	
	virtual void GetStringCommands( std::vector<std::string> *pCommands ) = 0;
	virtual bool LineEntered( const std::string &szLine, std::string *pszErr ) = 0;

	bool GetCommand( SCommand *pCmd );
};


