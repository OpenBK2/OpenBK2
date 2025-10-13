#pragma once

#include <cstdint>

namespace NLang
{
	struct IVisitor;

	class CLangNode : public CObjectBase
	{
		std::string szName;

		std::string szFileWhereDefined;
		int nLineWhereDefined;
	public:
		CLangNode() : nLineWhereDefined( -1 ) { }
		CLangNode( const std::string &_szName, const std::string &_szFileWhereDefined, int _nLineWhereDefined )
			: szName( _szName ), szFileWhereDefined( _szFileWhereDefined ), nLineWhereDefined( _nLineWhereDefined ) { }

		const std::string& GetFile() const { return szFileWhereDefined; }
		const int GetLine() const { return nLineWhereDefined; }

		const std::string& GetName() { return szName; }
		void SetName( const std::string &_szName ) { szName = _szName; }

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
		std::string szValue;
		uint32_t dwHexValue;
		std::string szName;
	public:
		CSimpleValue() : eType( EST_UNKNOWN ), bValue( false ), fValue( 0.0f ), dwHexValue( 0 ) { }
		CSimpleValue( const std::string &szValue, bool bString ) { SetValue( szValue, bString ); }
		void SetValue( const std::string &szValue, bool bString );
		void SetWStrValue( const std::string &_szValue )
		{
			szValue = _szValue;
			eType = EST_WSTRING;
		}

		void SetToEnum( const std::string &_szValue ) { szValue = _szValue; eType = EST_ENUM; }

		ESimpleType GetType() const { return eType; }
		const std::string& GetEnum() const { return szValue; }

		bool GetBool() const { return bValue; }
		float GetFloat() const { return fValue; }
		const std::string& GetString() const { return szValue; }
		uint32_t GetHexBinary() const { return dwHexValue; }
		int GetInt() const { return fValue; }
		uint16_t GetWORD() const { return fValue; }
		uint16_t GetDWORD() const { return dwHexValue; }
	};

	const char* GetTypeName( ESimpleType eType );
	ESimpleType GetType( const std::string &szTypeName );
	bool IsTypesEqual( ESimpleType eType1, ESimpleType eType2 );

	bool Parse( const std::string &szRootDir, const std::string &szFileMask, bool bInTestMode );
	bool Parse( const std::vector<std::string> &files, const std::string &szBaseFileName );
}


