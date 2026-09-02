#if !defined(__COMMON_TOOLS__MNEMONICS_COLLECTOR__)
#define __COMMON_TOOLS__MNEMONICS_COLLECTOR__
#pragma once


template <class TValue> class CMnemonicsCollector
{
	std::unordered_map<TValue, std::string> direct;
	std::unordered_map<std::string, TValue> reverse;

public:
	TValue defaultValue;
	std::string defaultMnemonic;
	//
	CMnemonicsCollector() {}
	CMnemonicsCollector( const TValue &rDefaultValue, const std::string &rszDefaultMnemonic )
		: defaultValue( rDefaultValue ), defaultMnemonic( rszDefaultMnemonic ) {}
	//
	void Clear( const TValue &rDefaultValue, const std::string &rszDefaultMnemonic )
	{
		defaultValue = rDefaultValue;
		defaultMnemonic = rszDefaultMnemonic;

		direct.clear();
		reverse.clear();
	}
	//
	void Insert( const TValue &rValue, const std::string &rszMnemonic )
	{
		direct[rValue] = rszMnemonic;
		reverse[rszMnemonic] = rValue;
	}
	//	
	int Size()
	{
		return direct.size();
	}
	//
	const std::string& GetMnemonic( const TValue &rValue ) const
	{
		typename std::unordered_map<TValue, std::string>::const_iterator it = direct.find( rValue );
		if ( it == direct.end() )
		{
			return defaultMnemonic;
		}
		else
		{
			return it->second;
		}
	}
	//
	const TValue &GetValue( const std::string &rszMnemonic ) const
	{
		typename std::unordered_map<std::string, TValue>::const_iterator it = reverse.find( rszMnemonic );
		if ( it == reverse.end() )
		{
			return defaultValue;
		}
		else
		{
			return it->second;
		}
	}
	//
	void Trace() const
	{
		DebugTrace( "mnemonic collector, begin" );
		for ( typename std::unordered_map<std::string, TValue>::const_iterator it = reverse.begin(); it != reverse.end(); ++it )
		{
			DebugTrace( "<%s>, %d", it->first.c_str(), it->second );
		}
		DebugTrace( "mnemonic collector, end" );
	}
};

#endif // #if !defined(__COMMON_TOOLS__SYS_CODES__)

