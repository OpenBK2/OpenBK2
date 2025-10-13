#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	enum ENotificationType;
	struct SComplexSoundDesc;
	struct STexture;
	struct SNotification;
	enum ENotificationEventType;
	enum EMinimapFigureType;

	enum ENotificationType
	{
		NTF_OBJECTIVE_RECEIVED = 0,
		NTF_OBJECTIVE_COMPLETED = 1,
		NTF_OBJECTIVE_FAILED = 2,
		NTF_KEY_OBJECT_CAPTURED = 3,
		NTF_KEY_OBJECT_LOSED = 4,
		NTF_ENEMY_ARTILLERY = 5,
		NTF_ENEMY_AA_FIRE = 6,
		NTF_REINFORCEMENT_ARRIVED = 7,
		NTF_OBJECTIVES_NOTIFY_REPEAT = 8,
		NTF_AVIA_BAD_WEATHER_RETREAT = 9,
		NTF_ENEMY_AVIA = 10,
		NTF_ENEMY_UNIT = 11,
		NTF_KEY_OBJECT_ATTACKED = 12,
		NTF_MINE_DETECTED = 13,
		NTF_UNIT_ATTACKED = 14,
		NTF_UNITS_GIVEN = 15,
		NTF_MINIMAP_FLARE = 16,
		NTF_COUNT = 17,
	};

	enum EMinimapFigureType
	{
		MFT_TRIANGLE = 0,
		MFT_SQUARE = 1,
		MFT_CIRCLE = 2,
	};

	struct SNotification : public CResource
	{
		OBJECT_BASIC_METHODS( SNotification )
	public:
		enum { typeID = 0x17135D40 };
		ENotificationType eType;
		NFile::CFilePath szTextFileRef;
		CDBPtr< SComplexSoundDesc > pSound;
		EMinimapFigureType eFigureType;
		int nColor;
		float fTime;
		float fSize;
		float fRotationSpeed;
		float fDuplicateDelay;

		#include "include_Notification.h"

		SNotification() :
			eType( NTF_OBJECTIVE_RECEIVED ),
			eFigureType( MFT_TRIANGLE ),
			nColor( 0 ),
			fTime( 0.0f ),
			fSize( 0.0f ),
			fRotationSpeed( 0.0f ),
			fDuplicateDelay( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	enum ENotificationEventType
	{
		NEVT_OBJECTIVE_COMPLETED = 0,
		NEVT_OBJECTIVE_RECEIVED = 1,
		NEVT_OBJECTIVE_FAILED = 2,
		NEVT_KEY_POINT_CAPTURED = 3,
		NEVT_KEY_POINT_ATTACKED = 4,
		NEVT_KEY_POINT_LOST = 5,
		NEVT_PLAYER_ELIMINATED = 6,
		NEVT_REINF_NEW_TYPE = 7,
		NEVT_REINF_LEVELUP = 8,
		NEVT_REINF_CANT_CALL = 9,
		NEVT_AVIA_AVAILABLE = 10,
		NEVT_AVIA_BAD_WEATHER_RETREAT = 11,
		NEVT_ENEMY_AVIA_DETECTED = 12,
		NEVT_ENEMY_ART_DETECTED = 13,
		NEVT_ENEMY_AA_DETECTED = 14,
		NEVT_ENEMY_UNIT_DETECTED = 15,
		NEVT_UNIT_ATTACKED = 16,
		NEVT_UNIT_BLOWUP_AT_MINE = 17,
		NEVT_UNIT_OUT_OF_AMMUNITION = 18,
		NEVT_ENGINEERING_MINE_DETECTED = 19,
		NEVT_ENGINEERING_COMPLETED = 20,
		NEVT_ENGINEERING_INTERRUPTED = 21,
		NEVT_OBJECTIVE_MOVED = 22,
		NEVT_UNITS_GIVEN = 23,
		NEVT_COUNT = 24,
	};

	struct SNotificationEvent : public CResource
	{
		OBJECT_BASIC_METHODS( SNotificationEvent )
	public:
		enum { typeID = 0x171BCB00 };
		ENotificationEventType eType;
		CDBPtr< STexture > pTexture;
		NFile::CFilePath szTextFileRef;
		NFile::CFilePath szTooltipFileRef;
		CDBPtr< SComplexSoundDesc > pSound;
		bool bShowByCamera;
		float fAutoRemoveTime;
		CDBPtr< SNotification > pNotification;
		float fNoDupArea;
		float fNoDupTime;

		#include "include_NotificationEvent.h"

		SNotificationEvent() :
			eType( NEVT_OBJECTIVE_COMPLETED ),
			bShowByCamera( false ),
			fAutoRemoveTime( 0.0f ),
			fNoDupArea( 0.0f ),
			fNoDupTime( 0.0f )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

namespace NDb
{
	std::string EnumToString( NDb::ENotificationType eValue );
	ENotificationType StringToEnum_NDb_ENotificationType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::ENotificationType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::ENotificationType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ENotificationType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_ENotificationType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMinimapFigureType eValue );
	EMinimapFigureType StringToEnum_NDb_EMinimapFigureType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMinimapFigureType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMinimapFigureType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMinimapFigureType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMinimapFigureType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::ENotificationEventType eValue );
	ENotificationEventType StringToEnum_NDb_ENotificationEventType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::ENotificationEventType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::ENotificationEventType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ENotificationEventType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_ENotificationEventType( szValue ); }
};

