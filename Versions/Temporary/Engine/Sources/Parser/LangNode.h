#pragma once

namespace NLang
{
	struct IVisitor;

	class CLangNode : public CObjectBase
	{
		string szName;

		string szFileWhereDefined;
		int nLineWhereDefined;
	public:
		CLangNode() : nLineWhereDefined( -1 ) { }
		CLangNode( const string &_szName, const string &_szFileWhereDefined, int _nLineWhereDefined )
			: szName( _szName ), szFileWhereDefined( _szFileWhereDefined ), nLineWhereDefined( _nLineWhereDefined ) { }

		const string& GetFile() const { return szFileWhereDefined; }
		const int GetLine() const { return nLineWhereDefined; }

		const string& GetName() { return szName; }
		void SetName( const string &_szName ) { szName = _szName; }

		virtual void Visit( IVisitor *pVisitor )
		{
			NI_ASSERT( false, "unknown visit" );
		}
	};
	
	enum ESimpleType
	{ 
		EST_UNKNOWN,
		EST_NOTYPE,
		EST_STRING,
		EST_HEXBINARY,
		EST_BOOL,
		EST_INT,
		EST_FLOAT,
		EST_WORD,
		EST_DWORD,
		EST_ENUM,
		EST_WSTRING,
	};

	class CSimpleValue
	{
		ESimpleType eType;
		bool bValue;
		float fValue;
		string szValue;
		DWORD dwHexValue;
		string szName;
	public:
		CSimpleValue() : eType( EST_UNKNOWN ), bValue( false ), fValue( 0.0f ), dwHexValue( 0 ) { }
		CSimpleValue( const string &szValue, bool bString ) { SetValue( szValue, bString ); }
		void SetValue( const string &szValue, bool bString );
		void SetWStrValue( const string &_szValue )
		{
			szValue = _szValue;
			eType = EST_WSTRING;
		}

		void SetToEnum( const string &_szValue ) { szValue = _szValue; eType = EST_ENUM; }

		ESimpleType GetType() const { return eType; }
		const string& GetEnum() const { return szValue; }

		bool GetBool() const { return bValue; }
		float GetFloat() const { return fValue; }
		const string& GetString() const { return szValue; }
		DWORD GetHexBinary() const { return dwHexValue; }
		int GetInt() const { return fValue; }
		WORD GetWORD() const { return fValue; }
		WORD GetDWORD() const { return dwHexValue; }
	};

	const char* GetTypeName( ESimpleType eType );
	ESimpleType GetType( const string &szTypeName );
	bool IsTypesEqual( ESimpleType eType1, ESimpleType eType2 );

	bool Parse( const string &szRootDir, const string &szFileMask, bool bInTestMode );
	bool Parse( const vector<string> &files, const string &szBaseFileName );
}


