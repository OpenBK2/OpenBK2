#pragma once

// automatically generated file, don't change manually!

#include "stats_b2_m1/dbmapinfo.h"
#include "stats_b2_m1/m1actions.h"
#include "stats_b2_m1/RPGStats.h"
#include "stats_b2_m1/Season.h"
#include "stats_b2_m1/useractions.h"
#include "UI/dbuiconsts.h"
#include "UI/dbuserinterface.h"
#include "System/FilePath.h"

struct IXmlSaver;

namespace NDb
{
	enum ESpecialAbilityParam;
	enum EUserAction;
	enum EM1Action;

	struct SARSetSpecialAbility : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSetSpecialAbility )
	public:
		enum { typeID = 0x1508D300 };
		EUserAction eAbility;
		ESpecialAbilityParam eAbilityParam;

		#include "include_arsetspecialability.h"

		SARSetSpecialAbility() :
			eAbility( USER_ACTION_UNKNOWN ),
			eAbilityParam( PARAM_ABILITY_ON )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SARSetForcedAction : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSetForcedAction )
	public:
		enum { typeID = 0x1508A340 };
		EUserAction eUserAction;
		EM1Action eM1UserAction;

		#include "include_arsetforcedaction.h"

		SARSetForcedAction() :
			eUserAction( USER_ACTION_UNKNOWN ),
			eM1UserAction( M1_ACTION_UNKNOWN )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	enum EReinforcementType;
	enum ESeason;
	enum EActionButtonPanel;
	enum EUserAction;
	struct STexture;
	enum EM1Action;
	struct SWindowMSButton;
	enum EMPGameType;
	struct SActionButtonInfo;

	enum EActionButtonPanel
	{
		ACTION_BTN_PANEL_DEFAULT = 0,
		ACTION_BTN_PANEL_ESC = 1,
		ACTION_BTN_PANEL_FORMATIONS = 2,
		ACTION_BTN_PANEL_RADIO_CONTROLLED = 3,
	};

	struct SActionButton
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EUserAction eAction;
		CDBPtr< SWindowMSButton > pButton;
		CDBPtr< SWindowMSButton > pNewButton;
		bool bIsAbility;
		bool bAutocast;
		bool bPassive;
		CVec2 vPos;
		NFile::CFilePath szTooltipFileRef;
		CDBPtr< STexture > pIcon;
		CDBPtr< STexture > pForegroundIcon;
		CDBPtr< STexture > pIconDisabled;
		CDBPtr< STexture > pForegroundIconDisabled;
		bool bPressEffect;
		EActionButtonPanel ePanel;
		EActionButtonPanel eTargetPanel;

		#include "include_ActionButton.h"

		SActionButton() :
			__dwCheckSum( 0 ),
			eAction( USER_ACTION_UNKNOWN ),
			bIsAbility( false ),
			bAutocast( false ),
			bPassive( false ),
			vPos( VNULL2 ),
			bPressEffect( false ),
			ePanel( ACTION_BTN_PANEL_DEFAULT ),
			eTargetPanel( ACTION_BTN_PANEL_DEFAULT )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SM1ActionButton
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EM1Action eAction;
		std::string szCrapName;
		CDBPtr< SWindowMSButton > pButton;
		CDBPtr< SWindowMSButton > pNewButton;
		bool bIsAbility;
		bool bAutocast;
		bool bPassive;
		CVec2 vPos;
		NFile::CFilePath szTooltipFileRef;
		CDBPtr< STexture > pIcon;
		CDBPtr< STexture > pForegroundIcon;
		CDBPtr< STexture > pIconDisabled;
		CDBPtr< STexture > pForegroundIconDisabled;
		bool bPressEffect;
		EActionButtonPanel ePanel;
		EActionButtonPanel eTargetPanel;

		#include "include_M1ActionButton.h"

		SM1ActionButton() :
			__dwCheckSum( 0 ),
			eAction( M1_ACTION_UNKNOWN ),
			bIsAbility( false ),
			bAutocast( false ),
			bPassive( false ),
			vPos( VNULL2 ),
			bPressEffect( false ),
			ePanel( ACTION_BTN_PANEL_DEFAULT ),
			eTargetPanel( ACTION_BTN_PANEL_DEFAULT )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SActionButtonInfo : public CResource
	{
		OBJECT_BASIC_METHODS( SActionButtonInfo )
	public:
		enum { typeID = 0x1717BAC0 };
		EUserAction eAction;
		bool bNoButton;
		bool bIsAbility;
		bool bAutocast;
		bool bPassive;
		int nSlot;
		EActionButtonPanel ePanel;
		EActionButtonPanel eTargetPanel;
		NFile::CFilePath szTooltipFileRef;
		CDBPtr< STexture > pIcon;
		CDBPtr< STexture > pIconDisabled;
		CDBPtr< STexture > pForegroundIcon;
		CDBPtr< STexture > pForegroundIconDisabled;
		bool bPressEffect;
		std::string szHotkeyCmd;

		#include "include_ActionButtonInfo.h"

		SActionButtonInfo() :
			eAction( USER_ACTION_UNKNOWN ),
			bNoButton( false ),
			bIsAbility( false ),
			bAutocast( false ),
			bPassive( false ),
			nSlot( 0 ),
			ePanel( ACTION_BTN_PANEL_DEFAULT ),
			eTargetPanel( ACTION_BTN_PANEL_DEFAULT ),
			bPressEffect( false )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SPlayersColors
	{
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SPlayer
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nColor;
			CDBPtr< SBackground > pUnitFullInfo;

			SPlayer() :
				__dwCheckSum( 0 ),
				nColor( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SUnitFullInfo
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SBackground > pUserForward;
			CDBPtr< SBackground > pNeutralForward;
			std::vector< CDBPtr< SBackground > > friendForwards;
			std::vector< CDBPtr< SBackground > > enemyForwards;

			SUnitFullInfo() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CVec3 vUserColor;
		CVec3 vNeutralColor;
		std::vector< CVec3 > friendColors;
		std::vector< CVec3 > enemyColors;
		SUnitFullInfo unitFullInfo;
		SPlayer userInfo;
		SPlayer neutralInfo;
		SPlayer friendInfo;
		SPlayer enemyInfo;

		SPlayersColors() :
			__dwCheckSum( 0 ),
			vUserColor( VNULL3 ),
			vNeutralColor( VNULL3 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinfButton
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eType;
		CDBPtr< SWindowMSButton > pButton;
		CDBPtr< STexture > pTexture;
		CDBPtr< STexture > pTextureDisabled;
		NFile::CFilePath szDescFileRef;

		SReinfButton() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SSeasonColor
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		ESeason eSeason;
		CVec3 vColor;

		SSeasonColor() :
			__dwCheckSum( 0 ),
			eSeason( SEASON_WINTER ),
			vColor( VNULL3 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMLTag
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::string szName;
		NFile::CFilePath szTextFileRef;

		#include "include_MLTag.h"

		SMLTag() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMPLocalizedGameType
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EMPGameType eGameType;
		NFile::CFilePath szLocalizedTextFileRef;

		#include "include_MPLocalizedGameType.h"

		SMPLocalizedGameType() :
			__dwCheckSum( 0 ),
			eGameType( MP_GT_STANDARD )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUIConstsB2 : public SUIGameConsts
	{
		OBJECT_BASIC_METHODS( SUIConstsB2 )
	public:
		enum { typeID = 0x1109C340 };

		struct SSeasonName
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			ESeason eSeason;
			NFile::CFilePath szNameFileRef;

			SSeasonName() :
				__dwCheckSum( 0 ),
				eSeason( SEASON_WINTER )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SActionButton > actionButtons;
		std::vector< SM1ActionButton > m1ActionButtons;
		std::vector< CDBPtr< SActionButtonInfo > > actionButtonInfos;
		SPlayersColors playersColors;
		std::vector< SReinfButton > reinfButtons;
		std::vector< SSeasonColor > chatSeasonColors;
		std::vector< SSeasonName > seasonNames;
		std::vector< SMLTag > tags;
		std::vector< SMPLocalizedGameType > mPLocalizedGameTypes;
		std::vector< CDBPtr< STexture > > chapterMapArrows;
		NFile::CFilePath szForbiddenWordsFileRef;

		#include "include_UIConstsB2.h"

		SUIConstsB2() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowMiniMapShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowMiniMapShared )
	public:
		enum { typeID = 0x1508E480 };
		CVec2 vPoint00;
		CVec2 vPoint01;
		CVec2 vPoint10;
		CVec2 vPoint11;
		std::vector< CVec3 > playerColors;
		CVec3 vViewportFrameColor;
		bool bRotable;
		CDBPtr< STexture > pRotableBackgroundTexture;
		CDBPtr< STexture > pRotableForegroundTexture;
		CVec2 vRotableBackgroundSize;
		CVec2 vRotableSize;

		SWindowMiniMapShared() :
			vPoint00( VNULL2 ),
			vPoint01( VNULL2 ),
			vPoint10( VNULL2 ),
			vPoint11( VNULL2 ),
			vViewportFrameColor( VNULL3 ),
			bRotable( false ),
			vRotableBackgroundSize( VNULL2 ),
			vRotableSize( VNULL2 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowMiniMap : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowMiniMap )
	public:
		enum { typeID = 0x1508E481 };

		#include "include_windowminimap.h"

		SWindowMiniMap() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SBackground;

	struct SWindowSelectionShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowSelectionShared )
	public:
		enum { typeID = 0x110BD482 };

		SWindowSelectionShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowSelection : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowSelection )
	public:
		enum { typeID = 0x110BD480 };
		CDBPtr< SBackground > pSelectorTexture;

		#include "include_windowselection.h"

		SWindowSelection() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct STexture;

	struct SWindowRoundProgressBarShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowRoundProgressBarShared )
	public:
		enum { typeID = 0x171713C0 };
		CDBPtr< STexture > pTexture;
		int nColor;

		SWindowRoundProgressBarShared() :
			nColor( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowRoundProgressBar : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowRoundProgressBar )
	public:
		enum { typeID = 0x171713C1 };

		#include "include_windowroundprogressbar.h"

		SWindowRoundProgressBar() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindow3DControlShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindow3DControlShared )
	public:
		enum { typeID = 0x17176480 };

		struct SObjectParams
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vPos;
			CVec2 vSize;

			SObjectParams() :
				__dwCheckSum( 0 ),
				vPos( VNULL2 ),
				vSize( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SObjectParams > places;

		SWindow3DControlShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindow3DControl : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindow3DControl )
	public:
		enum { typeID = 0x17176400 };

		#include "include_window3dcontrol.h"

		SWindow3DControl() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct STexture;

	struct SWindowFrameSequenceShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowFrameSequenceShared )
	public:
		enum { typeID = 0x1717A440 };
		CDBPtr< STexture > pTexture;
		CVec2 vFrameSize;
		int nFrameCountX;
		int nFrameCountY;
		int nTime;
		int nFrameCount;
		bool bRandomStartFrame;
		int nRandomAddTime;

		SWindowFrameSequenceShared() :
			vFrameSize( VNULL2 ),
			nFrameCountX( 0 ),
			nFrameCountY( 0 ),
			nTime( 0 ),
			nFrameCount( 0 ),
			bRandomStartFrame( false ),
			nRandomAddTime( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowFrameSequence : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowFrameSequence )
	public:
		enum { typeID = 0x1717A441 };

		#include "include_windowframesequence.h"

		SWindowFrameSequence() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SComplexSoundDesc;

	struct SUISPlaySound : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISPlaySound )
	public:
		enum { typeID = 0x11075C03 };
		CParam<CDBPtr<SComplexSoundDesc> > pSoundToPlay;

		#include "include_uisplaysound.h"

		SUISPlaySound() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUISB2MoveShared : public SUIStateBaseShared
	{
		OBJECT_BASIC_METHODS( SUISB2MoveShared )
	public:
		enum { typeID = 0x171B1C40 };
		CParam<CVec2> vOffset;
		CParam<CVec2> vAccel;
		CParam<float> fMoveTime;
		CParam<CVec2> vOffset2;
		CParam<CVec2> vAccel2;
		CParam<float> fMoveTime2;

		SUISB2MoveShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUISB2Move : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISB2Move )
	public:
		enum { typeID = 0x171B1C41 };
		CVec2 vOffset;
		CVec2 vAccelCoeff;
		float fMoveTime;
		CVec2 vOffsetBounce;
		CVec2 vAccelCoeffBounce;
		float fMoveTimeBounce;
		std::string szElementToMove;
		bool bBorder;
		float fMaxMoveTime;

		#include "include_uisb2move.h"

		SUISB2Move() :
			vOffset( VNULL2 ),
			vAccelCoeff( VNULL2 ),
			fMoveTime( 0.0f ),
			vOffsetBounce( VNULL2 ),
			vAccelCoeffBounce( VNULL2 ),
			fMoveTimeBounce( 0.0f ),
			bBorder( false ),
			fMaxMoveTime( 0.0f )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowPotentialLinesShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowPotentialLinesShared )
	public:
		enum { typeID = 0x191B53C0 };
		int nColour;

		SWindowPotentialLinesShared() :
			nColour( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowPotentialLines : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowPotentialLines )
	public:
		enum { typeID = 0x191B53C1 };

		#include "include_windowpotentiallines.h"

		SWindowPotentialLines() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct STexture;

	struct SBackgroundFrameSequence : public SBackground
	{
		OBJECT_BASIC_METHODS( SBackgroundFrameSequence )
	public:
		enum { typeID = 0x171C1B81 };
		CDBPtr< STexture > pSequenceTexture;
		CVec2 vFrameSize;
		int nFrameCountX;
		int nFrameCountY;
		int nTime;
		int nFrameCount;
		bool bRandomStartFrame;
		int nRandomAddTime;

		#include "include_backgroundframesequence.h"

		SBackgroundFrameSequence() :
			vFrameSize( VNULL2 ),
			nFrameCountX( 0 ),
			nFrameCountY( 0 ),
			nTime( 0 ),
			nFrameCount( 0 ),
			bRandomStartFrame( false ),
			nRandomAddTime( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
}

namespace NDb
{
	std::string EnumToString( NDb::EActionButtonPanel eValue );
	EActionButtonPanel StringToEnum_NDb_EActionButtonPanel( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EActionButtonPanel>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EActionButtonPanel eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EActionButtonPanel ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EActionButtonPanel( szValue ); }
};

