
#pragma once

// for adding to type converter enums in way STRING_ENUM_ADDER( converter, ENUM )
#define STRING_ENUM_ADD_PTR(TypeConverter,eEnum) (*TypeConverter)[#eEnum] = eEnum;
#define STRING_ENUM_ADD(TypeConverter,eEnum) TypeConverter.Add( #eEnum, eEnum );

template < class T1, class T2, class TH1 = std::hash<T1>, class TH2 = std::hash<T2> >
class CTypeConvertor
{
protected: // сериализовать хочется.
	std::unordered_map<T1, T2, TH1> t1_t2;
	std::unordered_map<T2, T1, TH2> t2_t1;
	//
	const T1& GetType1( const T2 &t2 ) const
	{
		typename std::unordered_map<T2, T1, TH2>::const_iterator pos = t2_t1.find( t2 );
		NI_ASSERT( pos != t2_t1.end(), "Can't find type1 from type2" );
		return pos->second;
	}
	const T2& GetType2( const T1 &t1 ) const
	{
		typename std::unordered_map<T1, T2, TH1>::const_iterator pos = t1_t2.find( t1 );
		NI_ASSERT( pos != t1_t2.end(), "Can't find type2 from type1" );
		return pos->second;
	}
public:
	//
	void Add( const T1 &t1, const T2 &t2 ) { t1_t2[t1] = t2; t2_t1[t2] = t1; }
	void Remove( const T1 &t1 ) { const T2 &t2 = GetType2( t1 ); t2_t1.erase( t2 ); t1_t2.erase( t1 ); }
	void Remove( const T2 &t2 ) { const T1 &t1 = GetType1( t2 ); t1_t2.erase( t1 ); t2_t1.erase( t2 ); }
	//
	const T2& ToT2( const T1 &t1 ) const { return GetType2( t1 ); }
	const T1& ToT1( const T2 &t2 ) const { return GetType1( t2 ); }
	bool IsPresent( const T1 &t1 ) const { return t1_t2.find( t1 ) != t1_t2.end(); }
	bool IsPresent( const T2 &t2 ) const { return t2_t1.find( t2 ) != t2_t1.end(); }

	bool IsEmpty() const { return t1_t2.empty() || t2_t1.empty(); }
	void Clear() { t1_t2.clear(); t2_t1.clear(); }
	
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &t1_t2 );
		saver.Add( 2, &t2_t1 );
		return 0;
	}
};


