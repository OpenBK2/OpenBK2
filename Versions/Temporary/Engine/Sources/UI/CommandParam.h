#pragma once


template <class TParamType>
class CParam : public pair<TParamType,bool>
{
	typedef pair<TParamType,bool> TParent;
public:
	CParam() { second = false; }
	CParam( const TParamType &par ) : pair<TParamType,bool>( par, true ){  }
	const CParam &operator=( const TParamType &par ) { pair<TParamType,bool>::operator =( pair<TParamType,bool>( par, true ) ); return *this; }

	void Merge( const CParam &par )
	{
		if ( !IsValid() )
			pair<TParamType,bool>::operator=( par );
	}

	bool IsValid() const { return second; }
	TParamType &Get() { return first; }
	const TParamType &Get() const { return first; }
	operator TParamType() { return first; }
	int operator&( IBinSaver &f ) { f.Add( 1, &first ); f.Add( 2, &second ); return 0; }
	DWORD CalcCheckSum() const { return 0; }
};
