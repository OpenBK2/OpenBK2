#include "stdafx.h"

#include "PropertyValues.h"
#include "PC_Constants.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/ObjectBaseController.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "MapEditorLib/StringManager.h"
#include "Misc/StrProc.h"

#include <fmt/format.h>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <list>

// The property value rules, moved here from the MFC item editors with their
// bodies unchanged: see PropertyValues.h.

namespace
{
	// The orders the combo editors sorted their lists in.
	struct CIntCompare
	{
		bool operator()( const std::string &rszText0, const std::string &rszText1 )
		{
			int nValue0 = 0;
			int nValue1 = 0;
			sscanf( rszText0.c_str(), "%d", &nValue0 );
			sscanf( rszText1.c_str(), "%d", &nValue1 );
			return ( nValue1 > nValue0 );
		}
	};


	struct CFloatCompare
	{
		bool operator()( const std::string &rszText0, const std::string &rszText1 )
		{
			float fValue0 = 0.0f;
			float fValue1 = 0.0f;
			sscanf( rszText0.c_str(), "%g", &fValue0 );
			sscanf( rszText1.c_str(), "%g", &fValue1 );
			return ( fValue1 > fValue0 );
		}
	};


	struct CStringCompare
	{
		bool operator()( const std::string &rszText0, const std::string &rszText1 )
		{
			return ( rszText1 > rszText0 );
		}
	};


	bool IntInputString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pszValue != 0, "IntInputString() pszValue == 0" );
		( *pszValue ) = std::to_string(  (int)rValue );
		return true;
	}


	bool IntInputValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pValue != 0, "IntInputValue() pValue == 0" );
		int nValue = 0;
		if ( sscanf( rszValue.c_str(), "%d", &nValue ) == 1 )
		{
			( *pValue ) = nValue;
			return true;
		}
		return false;
	}


	bool GuidString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pszValue != 0, "GuidString() pszValue == 0" );
		pszValue->clear();

		try
		{
			const boost::uuids::uuid *pValue = static_cast<const boost::uuids::uuid*>( rValue.GetPtr() );
			*pszValue = boost::uuids::to_string( *pValue );
		}
		catch ( ... )
		{
		}
		/*
		ASSERT( rValue.GetType() == CVariant::VT_GUID );
		*pszValue = rValue.ToString();
		*/

		return true;
	}


	bool GuidValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pValue != 0, "GuidValue() pValue == 0" );
		( *pValue ) = CVariant();
		uint8_t * pData = new uint8_t[sizeof( boost::uuids::uuid )];
		boost::uuids::uuid value;
		try
		{
			value = boost::uuids::string_generator()( rszValue );
			memcpy( pData, &value, sizeof( boost::uuids::uuid ) );
		}
		catch ( ... )
		{
		}
		( *pValue ) = CVariant( static_cast<void*>( pData ), pPropertyDesc->nSize );
		pValue->SetDestructorDeleted( ( pPropertyDesc->nSize > 0 ), pPropertyDesc->nSize );
		return true;
	}


	bool TextFileString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
	{
		( *pszValue ) = rValue.GetStringRecode();
		return true;
	}


	bool TextFileValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
	{
		( *pValue ) = rszValue;
		return true;
	}


	bool ExTextFileString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
	{
		( *pszValue ) = rValue.GetStringRecode();
		return true;
	}


	bool ExTextFileValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
	{
		( *pValue ) = rszValue;
		return true;
	}
}


namespace NPropertyValues
{
	bool BitFieldString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pszValue != 0, "BitFieldString() pszValue == 0" );
		pszValue->clear();
		if ( rValue.GetType() == CVariant::VT_POINTER )
		{
			const uint8_t *pValues = static_cast<const uint8_t*>( rValue.GetPtr() );
			for ( int nByteIndex = 0; nByteIndex < pPropertyDesc->nSize; ++nByteIndex )
				*pszValue += fmt::format( "{:02X}", pValues[nByteIndex] );
		}
		else if ( rValue.GetType() == CVariant::VT_INT )
		{
			const int nValue = (int)rValue;
			const uint8_t *pValues = reinterpret_cast<const uint8_t *>( &nValue );
			for ( int i = 0; i < 4; ++i )
				*pszValue += fmt::format( "{:02X}", pValues[i] );
		}
		else
		{
			NI_ASSERT( false, fmt::format("Can't convert type {} to bitfield", rValue.GetType()) );
		}
		return true;
	}


	bool BitFieldValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
	{
		NI_ASSERT( pValue != 0, "BitFieldValue() pValue == 0" );
		( *pValue ) = CVariant();
		uint8_t * pData = new uint8_t[pPropertyDesc->nSize];
		memset( pData, 0, pPropertyDesc->nSize );
		{
			uint8_t nHighByte = 0;
			uint8_t nLowByte = 0;
			int nByteIndex = 0;
			bool bLowByteAcquired = false;
			for ( int nCharIndex = 0; nCharIndex < rszValue.length(); ++nCharIndex ) 
			{
				if ( ( rszValue[nCharIndex] >= '0' ) && ( rszValue[nCharIndex] <= '9' ) )
				{
					nLowByte = rszValue[nCharIndex] - '0';
				}
				else if ( ( rszValue[nCharIndex] >= 'A' ) && ( rszValue[nCharIndex] <= 'F' ) )
				{
					nLowByte = rszValue[nCharIndex] - 'A' + 10;
				}
				else if ( ( rszValue[nCharIndex] >= 'a' ) && ( rszValue[nCharIndex] <= 'f' ) )
				{
					nLowByte = rszValue[nCharIndex] - 'a' + 10;
				}
				else
				{
					continue;
				}
				if ( bLowByteAcquired )
				{
					pData[nByteIndex] = ( nHighByte << 4 ) + nLowByte;
					++nByteIndex;
					if ( nByteIndex >= pPropertyDesc->nSize ) 
					{
						break;
					}
				}
				else
				{
					nHighByte = nLowByte;
				}
				bLowByteAcquired = !bLowByteAcquired;
			}
		}
		( *pValue ) = CVariant( static_cast<void*>( pData ), pPropertyDesc->nSize );
		pValue->SetDestructorDeleted( ( pPropertyDesc->nSize > 0 ), pPropertyDesc->nSize );
		return true;
	}


	// What CreateEditor put in the list, moved out of it unchanged.
	bool BuildIntChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices )
	{
		if ( pDesc == 0 || pChoices == 0 )
		{
			return false;
		}
		std::vector<std::string> &stringList = *pChoices;
		stringList.clear();
		//
		std::string szValues = pDesc->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		std::string szNumbers;
		if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_VALUES, 0, PCSP_STRONG_DIVIDERS, "", &szNumbers ) )
		{
			return false;
		}
		int	nStep = CStringManager::GetIntValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, 1 );
		if ( nStep <= 0 )
		{
			nStep = 1;
		}
		//
		int nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, 0 );
		while( nLeftPos != std::string::npos )
		{
			const int nRightPos = szNumbers.find_first_of( PCSP_SOFT_DIVIDERS, nLeftPos + 1 );
			const std::string szNumberList = szNumbers.substr( nLeftPos, nRightPos - nLeftPos );
			const int nRangePos = szNumberList.find_first_of( PCSP_RANGE_DIVIDERS );
			if ( nRangePos == std::string::npos )
			{
				int nValue = 0;
				if ( sscanf( szNumberList.c_str(), "%d", &nValue ) == 1 )
				{
					const std::string szValue = std::to_string(  nValue );
					stringList.push_back( szValue );
				}
				else
				{
					return false;
				}
			}
			else
			{
				const std::string szMinValue = szNumberList.substr( 0, nRangePos );
				const std::string szMaxValue = szNumberList.substr( nRangePos + 1 );
				int nMinValue = 0;
				int nMaxValue = 0;
				if ( ( sscanf( szMinValue.c_str(), "%d", &nMinValue ) == 1 ) &&
						 ( sscanf( szMaxValue.c_str(), "%d", &nMaxValue ) == 1 ) )
				{
					if ( nMinValue > nMaxValue )
					{
						const int nSwapValue = nMinValue;
						nMinValue = nMaxValue;
						nMaxValue = nSwapValue;
					}
					for ( int nValue = nMinValue; nValue <= nMaxValue; nValue += nStep )
					{
						const std::string szValue = std::to_string(  nValue );
						stringList.push_back( szValue );
					}
				}
			}
			if ( nRightPos != std::string::npos )
			{
				nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, nRightPos + 1 );
			}
			else
			{
				nLeftPos = std::string::npos;
			}
		}
		//
		if ( stringList.empty() )
		{
			return false;
		}
		//
		sort( stringList.begin(), stringList.end(), CIntCompare() );
		return true;
	}


	// What CreateEditor put in the list, moved out of it unchanged; the precision
	// it kept in nPrecision comes back through pnPrecision.
	bool BuildFloatChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices, int *pnPrecision )
	{
		if ( pDesc == 0 || pChoices == 0 || pnPrecision == 0 )
		{
			return false;
		}
		std::vector<std::string> &stringList = *pChoices;
		stringList.clear();
		int &nPrecision = *pnPrecision;
		//
		std::string szValues = pDesc->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		std::string szNumbers;
		if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_VALUES, 0, PCSP_STRONG_DIVIDERS, "", &szNumbers ) )
		{
			return false;
		}
		float	fStep = CStringManager::GetFloatValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, 1 );
		nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, nPrecision );
		if ( fStep <= 0.0f )
		{
			fStep = 1.0f;
		}
		if ( nPrecision > PCSV_MAX_RECISION )
		{
			nPrecision = PCSV_DEFAULT_RECISION;
		}
		else if ( nPrecision < 0 )
		{
			nPrecision = PCSV_DEFAULT_RECISION;
		}
		//
		int nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, 0 );
		while( nLeftPos != std::string::npos )
		{
			const int nRightPos = szNumbers.find_first_of( PCSP_SOFT_DIVIDERS, nLeftPos + 1 );
			const std::string szNumberList = szNumbers.substr( nLeftPos, nRightPos - nLeftPos );
			const int nRangePos = szNumberList.find_first_of( PCSP_RANGE_DIVIDERS );
			if ( nRangePos == std::string::npos )
			{
				float fValue = 0.0f;
				if ( sscanf( szNumberList.c_str(), "%g", &fValue ) == 1 )
				{
					const std::string szValue = CStringManager::GetFloatStringWithPrecision( fValue, nPrecision );
					stringList.push_back( szValue );
				}
				else
				{
					return false;
				}
			}
			else
			{
				const std::string szMinValue = szNumberList.substr( 0, nRangePos );
				const std::string szMaxValue = szNumberList.substr( nRangePos + 1 );
				float fMinValue = 0.0f;
				float fMaxValue = 0.0f;
				if ( ( sscanf( szMinValue.c_str(), "%g", &fMinValue ) == 1 ) &&
						 ( sscanf( szMaxValue.c_str(), "%g", &fMaxValue ) == 1 ) )
				{
					if ( fMinValue > fMaxValue )
					{
						const float fSwapValue = fMinValue;
						fMinValue = fMaxValue;
						fMaxValue = fSwapValue;
					}
					for ( float fValue = fMinValue; fValue <= fMaxValue; fValue += fStep )
					{
						const std::string szValue = CStringManager::GetFloatStringWithPrecision( fValue, nPrecision );
						stringList.push_back( szValue.c_str() );
					}
				}
			}
			if ( nRightPos != std::string::npos )
			{
				nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, nRightPos + 1 );
			}
			else
			{
				nLeftPos = std::string::npos;
			}
		}
		//
		if ( stringList.empty() )
		{
			return false;
		}
		//
		sort( stringList.begin(), stringList.end(), CFloatCompare() );
		return true;
	}


	// What CreateEditor put in the list after "null", moved out of it unchanged.
	void BuildRefChoices( const SPropertyDesc *pDesc, EPCIEType nType, std::vector<std::string> *pChoices )
	{
		if ( pDesc == 0 || pChoices == 0 )
		{
			return;
		}
		std::vector<std::string> stringList;
		//
		// The tables in the database.
		std::list<std::string> tables;
		if ( IResourceManager *pResourceManager = Singleton<IResourceManager>() )
		{
			if ( CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator() )
			{
				if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_CACHE_LOCAL ) )
				{
					std::string szTableName;
					while ( !pTableManipulatorIterator->IsEnd() )
					{
						pTableManipulatorIterator->GetName( &szTableName );
						tables.push_back( szTableName );
						pTableManipulatorIterator->Next();
					}
				}
			}
			// The objects of each table the reference may point at.
			for ( std::list<std::string>::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
			{
				if ( pDesc->refTypes.find( *itTable ) != pDesc->refTypes.end() )
				{
					if ( CPtr<IManipulator> pFolderManipulator = pResourceManager->CreateFolderManipulator( *itTable ) )
					{
						if ( CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_CACHE_LOCAL ) )
						{
							std::string szTableName;
							if ( typePCIEMnemonics.IsMultiRef( nType ) )
							{
								szTableName = *itTable + TYPE_SEPARATOR_CHAR;
							}
							std::string szName;
							while ( !pFolderManipulatorIterator->IsEnd() )
							{
								pFolderManipulatorIterator->GetName( &szName );
								if ( ( !szName.empty() ) &&
										 ( szName[szName.size() - 1] != PATH_SEPARATOR_CHAR ) )
								{
									stringList.push_back( szTableName + szName );
								}
								pFolderManipulatorIterator->Next();
							}
						}
					}
				}
			}
		}
		//
		sort( stringList.begin(), stringList.end(), CStringCompare() );
		pChoices->insert( pChoices->end(), stringList.begin(), stringList.end() );
	}


	bool GetVec3Color( int *pnColor, IManipulator *pManipulator, const std::string &rszName )
	{
		if ( pnColor && pManipulator )
		{
			bool bResult = true;
			float fR = 0.0f;
			float fG = 0.0f;
			float fB = 0.0f;
			//
			bResult = bResult && CManipulatorManager::GetValue( &fR, pManipulator, rszName + ".x" );
			bResult = bResult && CManipulatorManager::GetValue( &fG, pManipulator, rszName + ".y" );
			bResult = bResult && CManipulatorManager::GetValue( &fB, pManipulator, rszName + ".z" );
			//
			if ( bResult )
			{
				const int r = Clamp<int>( Clamp<float>( fR, 0.0f, 1.0f ) * 256.0f, 0, 255 );
				const int g = Clamp<int>( Clamp<float>( fG, 0.0f, 1.0f ) * 256.0f, 0, 255 );
				const int b = Clamp<int>( Clamp<float>( fB, 0.0f, 1.0f ) * 256.0f, 0, 255 );
				( *pnColor ) = (int)( ( 255 << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
			}
			return bResult;
		}
		return false;
	}


	bool AddVec3ColorChange( const std::string &rszName,const int nColor, CObjectBaseController *pObjectController, IManipulator *pManipulator )
	{
		if ( pObjectController && pManipulator )
		{
			const int r = ( nColor >> 16 ) & 0xFF;
			const int g = ( nColor >> 8 ) & 0xFF;
			const int b = nColor & 0xFF;
			//
			bool bResult = true;
			//
			float fR = Clamp<float>( Clamp<int>( r, 0, 255 ) / 255.0f, 0.0f, 1.0f );
			float fG = Clamp<float>( Clamp<int>( g, 0, 255 ) / 255.0f, 0.0f, 1.0f );
			float fB = Clamp<float>( Clamp<int>( b, 0, 255 ) / 255.0f, 0.0f, 1.0f );
			//	
			bResult = bResult && pObjectController->AddChangeOperation( rszName + ".x", fR, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( rszName + ".y", fG, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( rszName + ".z", fB, pManipulator );
			//
			return bResult;
		}
		return false;
	}
}


bool GetPCItemStringValue( std::string *pszValue,
													 const CVariant &rValue,
													 const std::string &rszDefaultValue,
													 EPCIEType nType,
													 const SPropertyDesc *pDesc,
													 bool bMultiline )
{
	NI_ASSERT( pszValue != 0, "GetPCItemStringValue(): pszValue == 0" );
	NI_ASSERT( pDesc != 0,  "GetPCItemStringValue(): pDesc == 0" );
	//
	if ( rValue.GetType() == CVariant::VT_MULTIVARIANT )
	{
		( *pszValue ) = "...";
		return true;
	}
	switch ( nType )
	{
		case PCIE_INT_INPUT:
				return IntInputString( pszValue, rValue, pDesc ); 
		case PCIE_INT_SLIDER:
		case PCIE_INT_COMBO:
			( *pszValue ) = std::to_string(  (int)rValue );
			return true;
		case PCIE_INT_COLOR:
		case PCIE_INT_COLOR_WITH_ALPHA:
		case PCIE_VEC3_COLOR:
		{
			const int nValue = (int)rValue;
			const int a = ( nValue >> 24 ) & 0xFF;
			const int r = ( nValue >> 16 ) & 0xFF;
			const int g = ( nValue >> 8 ) & 0xFF;
			const int b = nValue & 0xFF;
			if ( nType == PCIE_INT_COLOR_WITH_ALPHA )
			{
				( *pszValue ) = fmt::format( "{}, {}, {}, {}", a, r, g, b );
			}
			else
			{
				( *pszValue ) = fmt::format( "{}, {}, {}", r, g, b );
			}
			return true;
		}
		case PCIE_FLOAT_INPUT:
		case PCIE_FLOAT_SLIDER:
		case PCIE_FLOAT_COMBO:
		{
			std::string szValues = pDesc->szStringParam;
			NStr::ToLowerASCII( &szValues );
			int nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, PCSV_DEFAULT_RECISION );
			if ( nPrecision > PCSV_MAX_RECISION )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			else if ( nPrecision < 0 )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			( *pszValue ) = CStringManager::GetFloatStringWithPrecision( (float)rValue, nPrecision );
//			const std::string szFormat = fmt::format( "%.{}f", nPrecision );
//			( *pszValue ) = StrFmt( szFormat.c_str(), (float)rValue );
			return true;
		}
		case PCIE_BOOL_COMBO:
		case PCIE_BOOL_SWITCHER:
			( *pszValue ) = (bool)rValue ? PCSV_TRUE : PCSV_FALSE;
			return true;
		case PCIE_BOOL_CHECKBOX:
			( *pszValue ) = (bool)rValue ? PCSV_CHECK : PCSV_UNCHECK;
			return true;
		case PCIE_STRING_REF:
		case PCIE_STRING_COMBO_REF:
		case PCIE_STRING_MULTI_REF:
		case PCIE_STRING_COMBO_MULTI_REF:
		case PCIE_STRING_NEW_REF:
		case PCIE_STRING_NEW_MULTI_REF:
		if ( rValue.GetType() == CVariant::VT_NULL )
			{
				( *pszValue ) = PCSV_NULL;
			}
			else
			{
				( *pszValue ) = rValue.GetStringRecode();
			}
			return true;
		case PCIE_STRING_INPUT:
		case PCIE_STRING_COMBO:
		case PCIE_STRING_FILE_REF:
		case PCIE_STRING_DIR_REF:
			( *pszValue ) = rValue.GetStringRecode();
			return true;
		case PCIE_STRING_BIG_INPUT:
		{
			std::string szValue = rValue.GetStringRecode();
			if ( !bMultiline )
			{
				szValue = szValue.substr( 0, szValue.find_first_of( "\r\n" ) );
			}
			( *pszValue ) = szValue;
			return true;
		}
		case PCIE_BINARY_BIT_FIELD:
		{
			return NPropertyValues::BitFieldString( pszValue, rValue, pDesc ); 
		}
		case PCIE_GUID:
		{
			return GuidString( pszValue, rValue, pDesc ); 
		}
		case PCIE_TEXT_FILE:
		{
			return TextFileString( pszValue, rValue, pDesc ); 
		}
		case PCIE_NEW_TEXT_FILE:
		{
			return ExTextFileString( pszValue, rValue, pDesc ); 
		}
		default:
			break;
	}
	( *pszValue ) = rszDefaultValue;
	return false;
}


bool GetPCItemValue( CVariant *pValue,
										 const std::string &rszValue,
										 const CVariant &rDefaultValue,
										 EPCIEType nType,
										 const SPropertyDesc *pDesc )
{
	NI_ASSERT( pValue != 0, "GetPCItemValue(): pValue == 0" );
	NI_ASSERT( pDesc != 0,  "GetPCItemValue(): pDesc == 0" );
	//
	switch ( nType )
	{
		case PCIE_INT_INPUT:
				return IntInputValue( pValue, rszValue, pDesc );
		case PCIE_INT_SLIDER:
		case PCIE_INT_COMBO:
		{
			int nValue = 0;
			if ( sscanf( rszValue.c_str(), "%d", &nValue ) == 1 )
			{
				( *pValue ) = nValue;
				return true;
			}
			break;
		}
		case PCIE_INT_COLOR:
		case PCIE_INT_COLOR_WITH_ALPHA:
		case PCIE_VEC3_COLOR:
		{
			int a = 255;
			int r = 0;
			int g = 0;
			int b = 0;
			bool bScanned = false;
			if ( nType == PCIE_INT_COLOR_WITH_ALPHA )
			{
				if ( sscanf( rszValue.c_str(), "%d,%d,%d,%d", &a, &r, &g, &b ) == 4 )
				{
					if ( ( ( a >= 0 ) && ( a < 256 ) ) &&
							 ( ( r >= 0 ) && ( r < 256 ) ) &&
							 ( ( g >= 0 ) && ( g < 256 ) ) &&
							 ( ( b >= 0 ) && ( b < 256 ) ) )
					{
						bScanned = true;
					}
				}
			}
			else
			{
				if ( sscanf( rszValue.c_str(), "%d,%d,%d", &r, &g, &b ) == 3 )
				{
					if ( ( ( r >= 0 ) && ( r < 256 ) ) &&
							 ( ( g >= 0 ) && ( g < 256 ) ) &&
							 ( ( b >= 0 ) && ( b < 256 ) ) )
					{
						bScanned = true;
					}
				}
			}
			if ( bScanned )
			{
				( *pValue ) = (int)( ( a << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
				return true;
			}
			break;
		}
		case PCIE_FLOAT_INPUT:
		case PCIE_FLOAT_SLIDER:
		case PCIE_FLOAT_COMBO:
		{
			float fValue = 0.0f;
			if ( sscanf( rszValue.c_str(), "%g", &fValue ) == 1 )
			{
				( *pValue ) = fValue;
				return true;
			}
			break;
		}
		case PCIE_BOOL_COMBO:
		case PCIE_BOOL_SWITCHER:
			( *pValue ) = (bool)( rszValue == PCSV_TRUE );
			return true;
		case PCIE_BOOL_CHECKBOX:
			( *pValue ) = (bool)( rszValue == PCSV_CHECK );
			return true;
		case PCIE_STRING_REF:
		case PCIE_STRING_COMBO_REF:
		case PCIE_STRING_MULTI_REF:
		case PCIE_STRING_COMBO_MULTI_REF:
		case PCIE_STRING_NEW_REF:
		case PCIE_STRING_NEW_MULTI_REF:
			if ( rszValue.empty() || ( rszValue == PCSV_NULL ) )
			{
				( *pValue ) = CVariant();
			}
			else
			{
				( *pValue ) = rszValue;
			}
			return true;
		case PCIE_STRING_INPUT:
		case PCIE_STRING_COMBO:
		case PCIE_STRING_FILE_REF:
		case PCIE_STRING_DIR_REF:
		case PCIE_STRING_BIG_INPUT:
			( *pValue ) = rszValue;
			return true;
		case PCIE_BINARY_BIT_FIELD:
		{
			return NPropertyValues::BitFieldValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_GUID:
		{
			return GuidValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_TEXT_FILE:
		{
			return TextFileValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_NEW_TEXT_FILE:
		{
			return ExTextFileValue( pValue, rszValue, pDesc ); 
		}
		default:
			break;
	}
	( *pValue ) = rDefaultValue;
	return false;
}
