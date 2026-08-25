#pragma once

#include <cstdint>

template <class TParamType>
class CParam : public std::pair<TParamType,bool>
{
	typedef std::pair<TParamType,bool> TParent;
public:
	CParam() { this->second = false; }
	CParam( const TParamType &par ) : std::pair<TParamType,bool>( par, true ){  }
	const CParam &operator=( const TParamType &par ) { std::pair<TParamType,bool>::operator =( std::pair<TParamType,bool>( par, true ) ); return *this; }

	void Merge( const CParam &par )
	{
		if ( !IsValid() )
			std::pair<TParamType,bool>::operator=( par );
	}

	bool IsValid() const { return this->second; }
	TParamType &Get() { return this->first; }
	const TParamType &Get() const { return this->first; }
	operator TParamType() { return this->first; }
	int operator&( IBinSaver &f ) { f.Add( 1, &this->first ); f.Add( 2, &this->second ); return 0; }
	uint32_t CalcCheckSum() const { return 0; }
};

