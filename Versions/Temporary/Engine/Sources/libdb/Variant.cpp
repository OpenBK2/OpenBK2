#include "stdafx.h"

#include "Variant.h"
#include "Misc/StrProc.h"
#include "System/XmlSaver.h"

#define FLOAT_EPSILON 1e-5

// variant type

CVariant::CVariant( const CVariant &var ): m_pblob(NULL)
{
	Copy(var);
}

CVariant::~CVariant()
{
	Clear();
}

void CVariant::Clear()
{
	switch( m_eType ) {
		case VT_STR:          if ( m_pstr )  delete m_pstr; break;
		case VT_WSTR:         if ( m_pwstr ) delete m_pwstr; break;
		case VT_POINTER:      if ( m_pblob ) delete m_pblob; break;
		case VT_DBID:					if ( m_dbid )  delete m_dbid; break;
		case VT_MULTIVARIANT: if ( m_pmvar ) delete m_pmvar; break;
	default: break;
	}

	m_pblob = NULL;
	m_eType = VT_NULL;
}

void CVariant::Copy(const CVariant &var)
{
	Clear();

	m_eType = var.m_eType;
	switch( m_eType )
	{
	case VT_NULL:
		break;
	case VT_INT:
		m_int = var.m_int;
		break;
	case VT_FLOAT:
		m_float = var.m_float;
		break;
	case VT_STR:
		m_pstr = new std::string( *var.m_pstr );
		break;
	case VT_WSTR:
		m_pwstr = new std::wstring( *var.m_pwstr );
		break;
	case VT_BOOL:
		m_bool = var.m_bool;
		break;
	case VT_POINTER:
		m_pblob = new SBlob( *var.m_pblob );
		break;
	case VT_DBID:
		m_dbid = new CDBID( *var.m_dbid );
		break;
	case VT_MULTIVARIANT:
		m_pmvar = new SMultiVariant( *var.m_pmvar );
		break;
	default:
		ASSERT(0);
		break;
	}
}

CVariant& CVariant::operator=( const CVariant &var )
{
	if( &var != this )
	{
		Copy(var);
	}
	return *this;
}

bool CVariant::GetBool() const
{
	if(m_eType == VT_BOOL)
		return m_bool;
	NI_ASSERT( false, StrFmt("Can't convert type %d to bool", m_eType) );
	return false;
}

int CVariant::GetInt() const
{
	if ( m_eType == VT_INT )
		return m_int;
	// TODO change all places in code where NULL variant assumes (int)0
	NI_ASSERT( false, StrFmt("Can't convert type %d to int", m_eType) );
	return 0;
}

float CVariant::GetFloat() const
{
	if ( m_eType == VT_FLOAT )
		return m_float;
	NI_ASSERT( false, StrFmt("Can't convert type %d to float", m_eType) );
	return 0;
}

const char *CVariant::GetStr() const
{
	switch ( m_eType )
	{
	case VT_STR:
		return m_pstr->c_str();
	case VT_DBID:
		return m_dbid->ToString().c_str();
	default:
		NI_ASSERT( false, StrFmt("Can't convert type %d to string", m_eType) );
		return "";
	}
}

const wchar_t *CVariant::GetWStr() const
{
	if ( m_eType == VT_WSTR )
		return m_pwstr->c_str();
	NI_ASSERT( false, StrFmt("Can't convert type %d to wstring", m_eType) );
	return L"";
}

std::string CVariant::GetStringRecode() const
{
	switch ( m_eType )
	{
	case VT_STR:
		return *m_pstr;
	case VT_DBID:
		return m_dbid->ToString();
	case VT_WSTR:
		return NStr::ToMBCS( *m_pwstr );
	default:
		NI_ASSERT( false, StrFmt("Can't recode (!) type %d to string", m_eType) );
		return "";
	}
}

std::wstring CVariant::GetWStringRecode() const
{
	switch ( m_eType )
	{
	case VT_WSTR:
		return *m_pwstr;
	case VT_STR:
		return NStr::ToUnicode( *m_pstr );
	default:
		NI_ASSERT( false, StrFmt("Can't recode (!) type %d to wstring", m_eType) );
		return L"";
	}
}

const CDBID &CVariant::GetDBID() const
{
	static CDBID dummy;
	if ( m_eType == VT_DBID )
		return *m_dbid;
	NI_ASSERT( false, StrFmt("Can't convert type %d to CDBID", m_eType) );
	return dummy;
}

const void* CVariant::GetPtr() const
{
	NI_ASSERT( m_eType == VT_POINTER, "Unable to extract pointer from CVariant" );
	return m_pblob->m_ptr;
}

size_t CVariant::GetBlobSize() const
{
	NI_ASSERT( m_eType == VT_POINTER, "Unable to extract blob size from CVariant" );
	return m_pblob->m_nSize;
}

const CVariant::CMultiVariantMap* CVariant::GetMultiVariant() const
{
	NI_ASSERT( m_eType == VT_MULTIVARIANT, "Unable to extract MultiVariant from CVariant" );
	return m_pmvar->m_pMultiVariantMap;
}

void CVariant::SetMultiVariant( CMultiVariantMap *pMultiVariantMap, bool bDeleteInDestructor )
{
	Clear();
	m_eType = VT_MULTIVARIANT;
	m_pmvar = new SMultiVariant( pMultiVariantMap );
}

CVariant::operator int() const
{
	if ( m_eType == VT_INT )
	{
		return m_int;
	}
	else
	{
		switch ( m_eType )
		{
			case VT_FLOAT:
				return (int)m_float;
			case VT_STR:
				return NStr::ToInt( *m_pstr );
			case VT_BOOL:
				return m_bool;
			case VT_NULL:
				return 0;
			case VT_POINTER:
				if ( GetBlobSize() == 4 )
					return *( (int*)GetPtr() );
				else if ( GetBlobSize() == 0 )
					return 0;
				else
				{
					NI_ASSERT( false, StrFmt("Can't convert binary data of invalid size (%d) to int", GetBlobSize()) );
					return 0;
				}
			default:
//				NI_ASSERT( false, "CVariant: unable to convert to int" );
				return 0;
		}
	}
}


CVariant::operator float() const
{
	if ( m_eType == VT_FLOAT )
	{
		return m_float;	
	}
	else
	{
		switch ( m_eType )
		{
			case VT_INT:
				return m_int;
			case VT_STR:
				return NStr::ToFloat( *m_pstr );
			case VT_BOOL:
				return m_bool ? 1 : 0;
			default:
				NI_ASSERT( false, "CVariant: unable to convert to float" );
				return 0;
		}
	}
}


/*
CVariant::operator const string&() const
{
	NI_ASSERT( m_eType == VT_STR, "CVariant: unable to convert to string" );
	return m_str;
}

CVariant::operator const wstring&() const
{
	NI_ASSERT( m_eType == VT_WSTR, "CVariant: unable to convert to wstring" );
	return m_wstr;
}
*/

/*
CVariant::operator const char*() const
{
	NI_ASSERT( m_eType == VT_STR, "CVariant: unable to convert to string" );
	return GetStr();
}

CVariant::operator const wchar_t*() const
{
	NI_ASSERT( m_eType == VT_WSTR, "CVariant: unable to convert to wstring" );
	return GetWStr();
}
*/

CVariant::operator bool() const
{
	if ( m_eType == VT_BOOL )
	{
		return m_bool;
	}
	else
	{
		switch ( m_eType )
		{
			case VT_INT:
				return ( m_int != 0 );
			case VT_STR:
				return ( ( *m_pstr ) != "0" );
			case VT_POINTER:
				return ( m_pblob->m_ptr != 0 );
			case VT_MULTIVARIANT:
				return ( m_pmvar->m_pMultiVariantMap != 0 );
			default:
				NI_ASSERT( false, "CVariant: unable to convert to bool" );
				return false;
		}
	}
}

bool CVariant::ToText( std::string *pszText ) const
{
	if ( !pszText )
	{
		return false;
	}
	switch( m_eType )
	{
		case VT_NULL:
			( *pszText ) = StrFmt( "NULL" );
			break;
		case VT_INT:
			( *pszText ) = StrFmt( "%d", m_int );
			break;
		case VT_FLOAT:
			( *pszText ) = StrFmt( "%.4ff", m_float );
			break;
		case VT_STR:
			( *pszText ) = ( *m_pstr );
			break;
		case VT_WSTR:
			( *pszText ) = NStr::ToMBCS( *m_pwstr );
			break;
		case VT_BOOL:
			( *pszText ) = m_bool ? "true" : "false";
			break;
		case VT_POINTER:
			( *pszText ) = StrFmt( "0x%X", m_pblob->m_ptr );
			break;
		case VT_DBID:
			*pszText = m_dbid->ToString();
			break;
		case VT_MULTIVARIANT:
			( *pszText ) = "...";
			break;
		default:
			NI_ASSERT( false, "Uninitialized variant" );
	}
	return true;
}


bool CVariant::operator != ( const CVariant &var ) const
{
	return ! operator == (var);
	if ( this == &var )
	{
		return false;
	}
  else
	{
		switch( m_eType )
		{
		case VT_NULL:
			return var.m_eType != VT_NULL;
		case VT_INT:
			return ( m_int != (int)var );
		case VT_FLOAT:
			return ( m_float - (float)var > FLOAT_EPSILON );
		case VT_STR:
			return ( ( *m_pstr ) != var.GetStr() );
		case VT_WSTR:
			return ( ( *m_pwstr ) != var.GetWStr() );
		case VT_BOOL:
			return ( m_bool != (bool)var );
		case VT_POINTER:
			return true;
		case VT_DBID:
			return ( ( *m_dbid ) != var.GetDBID() );
		case VT_MULTIVARIANT:
			return true;
		default:
			NI_ASSERT( false, "Uninitialized variant" );
			return true;
		}
		return true;
	}
}

bool CVariant::operator == ( const CVariant &var ) const
{
	if ( this == &var )
		return true;
	else if ( (m_eType == VT_INT && var.m_eType == VT_POINTER) || (var.m_eType == VT_INT && m_eType == VT_POINTER) )
		return (int)(*this) == (int)var;
  else if ( m_eType != var.m_eType )
		return false;
  else
	{
		switch( m_eType )
		{
			case VT_NULL:
				return true;
			case VT_INT:
				return ( m_int == var.m_int );
			case VT_FLOAT:
				return ( m_float == var.m_float );
			case VT_STR:
				return ( ( *m_pstr ) == ( *var.m_pstr ) );
			case VT_WSTR:
				return ( ( *m_pwstr ) == ( *var.m_pwstr ) );
			case VT_BOOL:
				return ( m_bool == var.m_bool );
			case VT_POINTER:
				if( m_pblob->m_ptr && var.m_pblob->m_ptr && m_pblob->m_nSize == var.m_pblob->m_nSize )
				{
					return memcmp( m_pblob->m_ptr, var.m_pblob->m_ptr, m_pblob->m_nSize ) == 0;
				}
				else
					return false;
			case VT_DBID:
				return ( ( *m_dbid ) == ( *var.m_dbid ) );
			case VT_MULTIVARIANT:
				return false;
			default:
				NI_ASSERT( false, "Uninitialized variant" );
				return true;
		}
		return true;
	}
}

void CVariant::Trace() const
{
	if ( ( m_eType == VT_MULTIVARIANT ) && ( m_pmvar != 0 ) && ( m_pmvar->m_pMultiVariantMap != 0 ) )
	{
		for ( CMultiVariantMap::const_iterator itMultiVariant = m_pmvar->m_pMultiVariantMap->begin(); 
			    itMultiVariant != m_pmvar->m_pMultiVariantMap->end(); ++itMultiVariant )
		{
			DebugTrace( "%d", itMultiVariant->first );
		}
	}
}

int CVariant::operator & ( IBinSaver &saver )
{
	saver.Add( 1, &m_eType );

	switch( m_eType )
	{
	case VT_NULL:
		break;
	case VT_INT:
		saver.Add( 2, &m_int );
		break;
	case VT_FLOAT:
		saver.Add( 2, &m_float );
		break;
	case VT_BOOL:
		saver.Add( 2, &m_bool );
		break;
	case VT_STR:
		if ( saver.IsReading() ) m_pstr = new std::string();
		saver.Add( 2, m_pstr );
		break;
	case VT_WSTR:
		if ( saver.IsReading() ) m_pwstr = new std::wstring();
		saver.Add( 2, m_pwstr );
		break;
	case VT_POINTER:
		if ( saver.IsReading() ) m_pblob = new SBlob();
		saver.Add( 2, m_pblob );
		break;
	case VT_DBID:
		if ( saver.IsReading() ) m_dbid = new CDBID();
		saver.Add( 2, m_dbid );
		break;
	case VT_MULTIVARIANT:
		if ( saver.IsReading() ) m_pmvar = new SMultiVariant();
		saver.Add( 2, m_pmvar );
		break;
	}

	return 0;
}

int CVariant::operator&( IXmlSaver &saver )
{
	if ( saver.IsReading() )
		Clear();
	//
	saver.Add( "Type", &m_eType );
	switch ( m_eType )
	{
	case VT_NULL:
		break;

	case VT_INT:
		saver.Add( "Data", &m_int );
		break;

	case VT_FLOAT:
		saver.Add( "Data", &m_float );
		break;

	case VT_BOOL:
		saver.Add( "Data", &m_bool );
		break;

	case VT_STR:
		if ( saver.IsReading() )
			m_pstr = new std::string();
		saver.Add( "Data", m_pstr );
		break;

	case VT_WSTR:
		if ( saver.IsReading() )
			m_pwstr = new std::wstring();
		saver.Add( "Data", m_pwstr );
		break;

	case VT_POINTER:
		if ( saver.IsReading() ) 
			m_pblob = new SBlob();
		saver.Add( "Data", m_pblob );
		break;

	case VT_DBID:
		if ( saver.IsReading() ) 
			m_dbid = new CDBID();
		saver.Add( "Data", m_dbid );
		break;

	default:
		NI_ASSERT( false, StrFmt("Unsupported type %d to serialize!", m_eType) );
	}
	return 0;
}


