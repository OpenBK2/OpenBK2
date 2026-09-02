#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <fmt/printf.h>

#include "MapEditorLib_export.h"

//Класс для работы с Registry, только REG_SZ, REG_DWORD
//Возвращаемое значение - ERROR_SUCCES (0) или код GetLastError()
//Все внутренние ключи указываются относительно открытой секции, новые подкаталоги запрещены
class MAPEDITORLIB_EXPORT CRegistrySection
{
  protected:
	HKEY hRegistrySection;

  public:
  //Constructor
	//HKEY_CURRENT_USER
	//HKEY_LOCAL_MACHINE
  CRegistrySection( HKEY hKey, REGSAM samDesired, LPCTSTR pszRegistrySection );
  ~CRegistrySection();

	bool IsValid() { return ( hRegistrySection != 0 ); }

  //STL строка
	int32_t LoadString( LPCTSTR pszRegistryKey, std::string *pszLoadValue, const std::string &rszDefaultValue ) const;
  int32_t SaveString( LPCTSTR pszRegistryKey, const std::string &szSaveValue ) const;
 
	//uint32_t
	int32_t LoadDWORD( LPCTSTR pszRegistryKey, uint32_t *pdwLoadValue, uint32_t dwDefaultValue ) const;
  int32_t SaveDWORD( LPCTSTR pszRegistryKey, uint32_t dwSaveValue ) const;

	//Любое число ( только простые типы ), сохраняется и считывается по маске pszMask, хранится в виде строки для наглядности
	template<class TValue>
	int32_t LoadNumber( LPCTSTR pszRegistryKey, LPCTSTR pszMask, TValue *pLoadValue, const TValue rDefaultValue ) const
	{
		if ( pLoadValue != 0 )
		{
			( *pLoadValue ) = rDefaultValue;
			
			std::string szBuffer;
			int32_t eResult = LoadString( pszRegistryKey, &szBuffer, "" );
			if ( ( eResult == ERROR_SUCCESS ) && ( !szBuffer.empty() ) )
			{
				if ( sscanf( szBuffer.c_str(), pszMask, pLoadValue ) < 1 )
				{
					( *pLoadValue ) = rDefaultValue;
					eResult = ERROR_INVALID_DATA;
				}
			}
			return eResult;
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}

	template<class TValue>
  int32_t SaveNumber( LPCTSTR pszRegistryKey, LPCTSTR pszMask, const TValue &rSaveValue ) const
	{
		const std::string szBuffer = fmt::sprintf( pszMask, rSaveValue );
		return SaveString( pszRegistryKey, szBuffer );
	}

  //CTRect<TValue>, каждое поле сохраняется и считывается по маске pszMask,  хранится в виде строки для наглядности
	template<class TValue>
	int32_t LoadRect( LPCTSTR pszRegistryKey, LPCTSTR pszMask, CTRect<TValue> *pLoadValue, const CTRect<TValue> &rDefaultValue ) const
	{
		if ( pLoadValue != 0 )
		{
			( *pLoadValue ) = rDefaultValue;
			
			std::string szBuffer;
			int32_t eResult = LoadString( pszRegistryKey, &szBuffer, "" );
			if ( ( eResult == ERROR_SUCCESS ) && ( !szBuffer.empty() ) )
			{
				if ( sscanf( szBuffer.c_str(),
										 fmt::format( "{} {} {} {}", pszMask, pszMask, pszMask, pszMask ).c_str(),
										 &( pLoadValue->minx ),
										 &( pLoadValue->miny ),
										 &( pLoadValue->maxx ),
										 &( pLoadValue->maxy ) ) < 4 )
				{
					( *pLoadValue ) = rDefaultValue;
					eResult = ERROR_INVALID_DATA;
				}
			}
			return eResult;
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}

	template<class TValue>
  int32_t SaveRect( LPCTSTR pszRegistryKey, LPCTSTR pszMask, const CTRect<TValue> &rSaveValue ) const
	{
		const std::string szFormat = fmt::format( "{} {} {} {}", pszMask, pszMask, pszMask, pszMask );
		const std::string szBuffer = fmt::sprintf( szFormat.c_str(), rSaveValue.minx, rSaveValue.miny, rSaveValue.maxx, rSaveValue.maxy );
		return SaveString( pszRegistryKey, szBuffer );
	}
};




