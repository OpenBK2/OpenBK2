
// This source file is part of the Script (Lua Wrapper) source distribution
// and is Copyright 2000 by Joshua C. Jensen (jjensen@workspacewhiz.com).
// The latest version may be obtained from http://www.workspacewhiz.com/.
//
// The code presented in this file may be freely used and modified for all
// non-commercial and commercial purposes.

#include "stdafx.h"
#include "Script.h"

#if BOOST_OS_WINDOWS
#include <windows.h>
#endif

#include <cstdio>
#include <cstring>

#include "Misc/StrProc.h"

#include "lstate.h"


namespace NScript
{
	CObjectBase* luaGetPtr( const Script::Object &o );
}

static int Script_LOG(lua_State* state)
{
	Script script(state);
	Script::Object obj = script.GetObject(script.GetTop());
	// Was its own copy of the same platform split. Off Windows this now reaches
	// stderr rather than stdout, which is where the rest of the engine's tracing
	// goes and keeps it apart from what the game itself prints.
	DebugTrace( "%s", obj.GetString() );
	return 0;
}

char *pErrToString[] = 
{
	"OK",
	"Error while running the chunk",
	"Error opening the file",
	"Syntax error during pre-compilation",
	"Memory allocation error",
	"LUA_ERRERR",
};

const char *ErrorToString( int nErrorCode )
{
	if ( nErrorCode > LUA_ERRERR || nErrorCode < 0 )
		return "Unknown";
	return pErrToString[nErrorCode];
}

int Script::RegisterNewTag( const SRegFunction *pList )
{
	const SRegFunction *pCur = pList;
	int nTag = NewTag();
	for(; pCur->func && pCur->name; ++pCur )
	{
		PushCFunction( pCur->func );
		std::string str( pCur->name );
		str += nTag;
		lua_RegisterFunc( pCur->func, MakeUniqueName(m_state, str.c_str()) );
		SetTagMethod( nTag, pCur->name );
	}
	return nTag;
}


// lua_register
void Script::Register( const char* funcName, lua_CFunction function )
{
	lua_register(m_state, funcName, function);
}

void Script::Register( const SRegFunction *pList )
{
	for( const SRegFunction *p = pList; p->func && p->name; ++p )
		Register( p->name, p->func );
}

void Script::RegisterSaveLoad( const SRegFunction *pList )
{
	for( const SRegFunction *p = pList; p->func && p->name; ++p )
		lua_RegisterFunc( p->func, MakeUniqueName(m_state, p->name) );
}


static void luaErrorNEA( Script *pScript, const std::string &szFuncName )
{
	std::string str = "Not enough arguments when calling function ";
	str += szFuncName;
	str += ".\n";
	pScript->Error( str.c_str() );
}

static void luaErrorWToA( Script *pScript, const std::string &szFuncName, int nArg )
{
	std::string str = "Wrong type of argument ";
	str += char( '0' + nArg );
	str += ", when calling function ";
	str += szFuncName;
	str += ".\n";
	pScript->Error( str.c_str() );
}

static void luaWarningNVA( Script *pScript, const std::string &szFuncName, int nArg )
{
	std::string str = "Script warning: Argument ";
	str += char( '0' + nArg );
	str += " is no more valid, when calling function ";
	str += szFuncName;
	csSystem << str << endl;
}

bool Script::CheckArgs( const char *szArgList, const std::string &sFuncName, std::vector<SLuaParams> *pParams, bool bLog )
{
	pParams->clear();
	const char *pCurChar = szArgList;
	int nCurArg = 1;
	int nArgs = GetTop();
	//
	bool bDone = false;
	//
	while ( !bDone )
	{
		char cTypeID = *pCurChar;
		std::string szDefaultValue = "";
		bool isOK = true;
		bool bHasDefaultValue = false;
		//
		++pCurChar;
		if ( *pCurChar == '[' )
		{
			for ( ++pCurChar; *pCurChar != ']' && *pCurChar != char( 0 ); ++pCurChar )
				szDefaultValue += *pCurChar;
			if ( *pCurChar != char( 0 ) )
				++pCurChar;
			bHasDefaultValue = true;
		}
		bDone = *pCurChar == char( 0 );
		//
		pParams->resize( pParams->size() + 1 );
		SLuaParams &param = pParams->back();
		//
		if ( nCurArg <= nArgs )
		{
			Object o = GetObject( nCurArg );
			switch( cTypeID )
			{
				case 'n':
					isOK = o.IsNumber();
					if ( isOK )			
					{
						param.f = o.GetNumber();
						param.n = param.f;
						if ( bLog )
							param.s = std::string( o.GetString() );
					}
					break;
				case 's':
					isOK = o.IsString();
					if ( isOK )
						param.s = o.GetString();
					break;
				case 't':
					isOK = o.IsTable();
					if ( bLog )
						param.s = "[table]";
					break;
				case 'u':
					isOK = o.IsUserData();
					if ( isOK )
					{
						param.p = NScript::luaGetPtr( o );
						if ( !IsValid( param.p ) )
						{
							param.p = 0;
							luaWarningNVA( this, sFuncName, nCurArg );
							//isOK = false;
						}
					}
					break;
				case 'b':
					isOK = o.IsNumber() || o.IsNil();
					if ( isOK )
						param.b = !o.IsNil();
					if ( bLog )
						param.s = param.b ? "true" : "false";
					break;
				case '.':
					return true;
					break;
			}
			if ( !isOK )
			{
				luaErrorWToA( this, sFuncName, nCurArg );
				return false;
			}
		}
		else if ( bHasDefaultValue )
		{
			// параметры по умолчанию
			switch( cTypeID )
			{
				case 'n':
					param.f = NStr::ToFloat( szDefaultValue );
					param.n = param.f;
					break;
				case 's':
					param.s = szDefaultValue;
					break;
				case 'b':
					if ( szDefaultValue == "false" )
						param.b = false;
					else
						param.b = true;
					break;
			}
		}
		else
		{
			luaErrorNEA( this, sFuncName );
			return false;
		}
		//
		++nCurArg;
	}
	if ( *pCurChar != 0 )
	{
		luaErrorNEA( this, sFuncName );
		return false;
	}
	return true;
}

Script::Script( const char* instanceName, bool initStandardLibrary, lua_CFunction logFunction )
	: m_ownState(false), nPushCount( 0 ) 
{
	m_state = lua_open(0, instanceName);
	m_ownState = true;

	// Register some basic functions with Lua.
//	Register("LOG", Script_LOG);
	if ( !logFunction )
		Register("_ERRORMESSAGE", Script_LOG);
	else
		Register("_ERRORMESSAGE", logFunction);
}


/**
	@return Retrieves the value at [section].[entry].  If either
		[section] or [entry] doesn't exist, [defaultValue] is returned.
**/
int Script::ConfigGetInteger(const char* section, const char* entry,
							 int defaultValue)
{
	return static_cast<int>(ConfigGetReal(section, entry, defaultValue));
}


/**
	@return Retrieves the value at [section].[entry].  If either
		[section] or [entry] doesn't exist, [defaultValue] is returned.
**/
float Script::ConfigGetReal(const char* section, const char* entry,
							double defaultValue)
{
	AutoBlock block(*this);

	Object obj = GetGlobal(section);
	if (obj.IsNil())
		return (float)defaultValue;
	obj = obj.GetByName(entry);
	if (obj.IsNumber())
		return obj.GetNumber();
	return (float)defaultValue;
}


/**
	@return Retrieves the value at [section].[entry].  If either
		[section] or [entry] doesn't exist, [defaultValue] is returned.
**/
const char* Script::ConfigGetString(const char* section, const char* entry,
									const char* defaultValue)
{
	AutoBlock block(*this);

	Object obj = GetGlobal(section);
	if (obj.IsNil())
		return defaultValue;
	obj = obj.GetByName(entry);
	if (obj.IsString())
		return obj.GetString();
	return defaultValue;
}


/**
	Assigns [value] to [section].[entry].
**/
void Script::ConfigSetInteger(const char* section, const char* entry, int value)
{
	AutoBlock block(*this);

	// section.entry = value
	// Difficult in code.  Do it this way.
	Object sectionTable = GetGlobal(section);

	// If the global table isn't there, then create it.
	if (sectionTable.IsNil())
	{
		sectionTable = GetGlobals().CreateTable(section);
	}

	sectionTable.SetNumber(entry, value);
}


/**
	Assigns [value] to [section].[entry].
**/
void Script::ConfigSetReal(const char* section, const char* entry, double value)
{
	AutoBlock block(*this);

	// section.entry = value
	// Difficult in code.  Do it this way.
	Object sectionTable = GetGlobal(section);

	// If the global table isn't there, then create it.
	if (sectionTable.IsNil())
	{
		sectionTable = GetGlobals().CreateTable(section);
	}

	sectionTable.SetNumber(entry, value);
}


/**
	Assigns [value] to [section].[entry].
**/
void Script::ConfigSetString(const char* section, const char* entry, const char* value)
{
	AutoBlock block(*this);

	// section.entry = value
	// Difficult in code.  Do it this way.
	Object sectionTable = GetGlobal(section);

	// If the global table isn't there, then create it.
	if (sectionTable.IsNil())
	{
		sectionTable = GetGlobals().CreateTable(section);
	}

	sectionTable.SetString(entry, value);
}

static std::string IndentString( unsigned int indentLevel )
{
	std::string str;
	for ( int i = 0; i < indentLevel; ++i )
		str += ' ';
	return str;
}

std::string Script::GetObjectAsText( const char* name, Script::Object value, bool bDecorateOutput, unsigned int indentLevel )
{
	Script& script = *this;
	std::string strRet;

	// Indent the line the number of spaces for the current indentation level.
	const unsigned int INDENT_SIZE = 2;
	const unsigned int indentSpaces = indentLevel * INDENT_SIZE;
	strRet += IndentString(indentSpaces);
	
	if (name)
	{
		strRet += name;
		strRet += " = ";
	}

	if ( value.IsUserData() )
		strRet += "<userdata>";
	else if ( value.IsCFunction() )
		strRet += "<C-function>";
	else if ( value.IsFunction() )
		strRet += "<function>";
	else if ( value.IsNil() )
		strRet += "nil";
	else if ( value.IsNumber() )
	{
		char buff[1024];
		sprintf( buff, "%.16g", value.GetNumber() );
		strRet += buff;
	}
	else if ( value.IsString() )
	{
		if ( bDecorateOutput )
			strRet += '"';
		strRet += value.GetString();
		if ( bDecorateOutput )
			strRet += '"';
	}
	else if (value.IsTable())
	{
		// Write the table header.
		strRet += "\n";
		strRet += IndentString(indentSpaces);
		strRet += "{\n";

		// Rename, just for ease of reading.
		Script::Object table = value;

		// upperIndex is the upper index value of a sequential numerical array
		// items.
		int upperIndex = 1;
		bool wroteSemi = false;
		bool hasSequential = false;

		// Block to search for array items.
		{
			// Pop the stack state when done.
			Script::AutoBlock block(script);

			// Grab index 1 and index 2 of the table.
			Object value1 = table.GetByTableIndex(1);
			Object value2 = table.GetByTableIndex(2);

			// If they both exist, then there is a sequential list.
			if (!value1.IsNil()  &&  !value2.IsNil())
			{
				// Cycle through the list.
				bool firstSequential = true;
				for ( ; ; ++upperIndex )
				{
					// Restore the stack state each iteration.
					Script::AutoBlock block(script);

					// Try retrieving the table entry at upperIndex.
					Object value = table.GetByTableIndex(upperIndex);

					// If it doesn't exist, then exit the loop.
					if ( value.IsNil() )
						break;

					// Only add the comma and return if not on the first item.
					if ( !firstSequential )
						strRet += "\n";
					
					// Write the object as an unnamed entry.
					strRet += GetObjectAsText( NULL, value, indentLevel + 1 );

					// We've definitely passed the first item now.
					firstSequential = false;
				}
			}
		}

		// Did we find any sequential table values?
		if (upperIndex > 1)
		{
			hasSequential = true;
		}
		
		// Cycle through the table.
		int i;
		script.PushNil();
		script.PushNil();
		while ( (i = table.Next()) != 0 )
		{
			char keyName[255];

			// Retrieve the table entry's key and value.
			Object key = script.GetObject(script.GetTop() - 1);
			Object value = script.GetObject(script.GetTop());

			// Is the key a number?
			if (key.IsNumber())
			{
				// Yes, were there sequential array items in this table?
				if (hasSequential)
				{
					// Is the array item's key an integer?
					float realNum = key.GetNumber();
					int intNum = (int)realNum;
					if (realNum == (float)intNum)
					{
						// Yes.  Is it between 1 and upperIndex?
						if (intNum >= 1  &&  intNum < upperIndex)
						{
							// We already wrote it as part of the sequential
							// list.
							continue;
						}
					}
				}

				// Build the table entry name for the number.
				sprintf(keyName, "[%.16g]", key.GetNumber());
			}
			else
			{
				// Build the table entry name for the string key name.
				strcpy(keyName, key.GetString());
			}

			// If we wrote a sequential list, the value we're about to write
			// is not nil, and we haven't written the semicolon to separate
			// the sequential table entries from the keyed table entries...
			if (hasSequential  &&  !value.IsNil()  &&  !wroteSemi)
			{
				// Then add a comma (for good measure) and the semicolon.
				strRet += ", ;\n";
				wroteSemi = true;
			}

			// Write the table entry.
			strRet += GetObjectAsText( keyName, value, indentLevel + 1 );

			// Add a comma after the table entry.
			strRet += ",\n";
		}

		// If we wrote a sequential list and haven't written a semicolon, then
		// there were no keyed table entries.  Just write the final comma.
		if (hasSequential  &&  !wroteSemi)
		{
			strRet += ",\n";
		}
		
		// Indent, with the intent of closing up the table.
		strRet += IndentString(indentSpaces);

		// Close the table.  The comma is written when WriteObject()
		// returns from the recursive call.
		strRet += "}";
	}
	if (indentLevel == 0 && bDecorateOutput)
		strRet += "\n";
	return strRet;
}

std::string Script::GetObjectAsText( const char* name, bool bDecorateOutput )
{
	std::string strRet;
	AutoBlock block(*this);
	GetGlobal(name);
	Object value = GetObject(GetTop());
	if ( !value.IsNil() )
		return GetObjectAsText( name, value, bDecorateOutput, 0 );
	else
		return std::string("There is no global entity with name \"") + name + "\"";
}

std::string Script::GetStateAsText()
{
	std::string str;
	// For safety, just in case we leave something behind on the script stack.
	AutoBlock block(*this);

	// Run through all the globals.
	str+="Globals: \n";
	int i;
	Object table = GetGlobals();
	PushNil();
	PushNil();
	while ( ( i = table.Next() ) != 0 )
	{
		// Retrieve the global's key and value.
		Object key = GetObject(GetTop() - 1);
		Object value = GetObject(GetTop());

		// Save the global to the text file.
		if ( strcmp(key.GetString(), "_VERSION" ) != 0 )
		{
			str += GetObjectAsText( key.GetString(), value, 0 );
		}
	}
	char buf[30];
	int nThreads = m_state->threads.size();
	sprintf( buf, "Threads number = %zu \n", m_state->threads.size() );
	str+=buf;
	for ( CThreads::iterator i = m_state->threads.begin(); i != m_state->threads.end(); ++i )
	{
		CLuaThread &tred = **i;
		sprintf( buf, "Thread #%p: stacksize %d", i->GetPtr(), tred.top );
		str += buf;
		if ( tred.top > 0 )
		{
			str += ", elemtypes:";
			for (StkId id = 0; id < tred.top; ++id )
			{
				sprintf( buf, " %d", LObj( m_state, id )->GetType() );
				str += buf;
			}
		}
		sprintf( buf, "\n" );
		str+=buf;
	}
	return str;
}

int Script::operator&( IBinSaver &f ) 
{ 
	if ( f.IsReading() )
	{
		if ( m_ownState )
			lua_close( m_state ); 
		f.Add( 2, &m_ownState );
		ASSERT( m_ownState );
		m_state = lua_open(0);
		f.Add( 3, m_state ); // все правильно, амперсенд не нужен
	}
	else
	{
		ASSERT( m_ownState ); // не умеем сериалайзить, если стейт не свой
		f.Add( 2, &m_ownState );
		f.Add( 3, m_state ); // все правильно, амперсенд не нужен
	}
	return 0; 
}


