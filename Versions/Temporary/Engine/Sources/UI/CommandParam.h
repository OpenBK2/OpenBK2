#pragma once


template <class TParamType>
class CParam : public std::pair<TParamType,bool>
{
	typedef std::pair<TParamType,bool> TParent;
public:
	CParam() { second = false; }
	CParam( const TParamType &par ) : std::pair<TParamType,bool>( par, true ){  }
	const CParam &operator=( const TParamType &par ) { std::pair<TParamType,bool>::operator =( std::pair<TParamType,bool>( par, true ) ); return *this; }

	void Merge( const CParam &par )
	{
		if ( !IsValid() )
			std::pair<TParamType,bool>::operator=( par );
	}

	bool IsValid() const { return second; }
	TParamType &Get() { return first; }
	const TParamType &Get() const { return first; }
	operator TParamType() { return first; }
	int operator&( IBinSaver &f ) { f.Add( 1, &first ); f.Add( 2, &second ); return 0; }
	DWORD CalcCheckSum() const { return 0; }
};

