#pragma once

#include "UI_export.h"


// automatically generated file, don't change manually!

#include "commandparam.h"
#include "../system/filepath.h"

interface IXmlSaver;

namespace NDb
{
	enum EPositionAllign;
	struct SUIStateBase;
	struct SUIStateBaseShared;
	struct SForegroundTextString;
	struct SWindowMSButton;

	enum EPositionAllign
	{
		EPA_LOW_END = 0,
		ERA_CENTER = 1,
		EPA_HIGH_END = 2,
		EPA_MARGIN = 3,
	};

	struct UI_EXPORT SUIDesc : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nClassTypeID;

		SUIDesc() :
			__dwCheckSum( 0 ),
			nClassTypeID( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUICommandBase
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CParam<string> szParam1;
		CParam<string> szParam2;
		CParam<CVec2> vParam1;
		CParam<int> nParam1;

		SUICommandBase() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBUIMessage
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szMessageID;
		string szStringParam;
		int nIntParam;

		SBUIMessage() :
			__dwCheckSum( 0 ),
			nIntParam( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct UI_EXPORT SUIStateBaseShared : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		SUIStateBaseShared() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct UI_EXPORT SUIStateBase : public SUIDesc
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SUIStateBaseShared > ppShared;

		SUIStateBase() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUISMoveTo : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISMoveTo )
	public:
		enum { typeID = 0x11075C00 };
		CParam<CVec2> vOffset;
		CParam<float> fMoveTime;
		CParam<string> szElementToMove;

		#include "include_uismoveto.h"

		SUISMoveTo() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUISRunReaction : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISRunReaction )
	public:
		enum { typeID = 0x11075C05 };
		CParam<string> szReactionForward;
		CParam<string> szReactionBack;

		#include "include_uisrunreaction.h"

		SUISRunReaction() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUISSendUIMessage : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISSendUIMessage )
	public:
		enum { typeID = 0x11075C07 };
		CParam<string> szMessageID;
		CParam<string> szParam;
		CParam<int> nForwardParam;
		CParam<int> nBackParam;

		#include "include_uissenduimessage.h"

		SUISSendUIMessage() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUIConsoleCommand : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUIConsoleCommand )
	public:
		enum { typeID = 0x110953C1 };
		string szEditBoxName;

		#include "include_uiconsolecommand.h"

		SUIConsoleCommand() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUISDirectRunReaction : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISDirectRunReaction )
	public:
		enum { typeID = 0x210C5480 };
		CDBPtr< SUIDesc > pReactionForward;
		CDBPtr< SUIDesc > pReactionBackward;

		#include "include_uisdirectrunreaction.h"

		SUISDirectRunReaction() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SUIStateSequence
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SUIStateBase > > commands;
		bool bReversable;

		SUIStateSequence() :
			__dwCheckSum( 0 ),
			bReversable( false )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
	struct STexture;

	struct UI_EXPORT SBackground : public SUIDesc
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< STexture > pTexture;
		int nColor;

		SBackground() :
			__dwCheckSum( 0 ),
			nColor( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBackgroundSimpleScallingTexture : public SBackground
	{
		OBJECT_BASIC_METHODS( SBackgroundSimpleScallingTexture )
	public:
		enum { typeID = 0x1106CB03 };
		CVec2 vSize;

		#include "include_backgroundsimplescallingtexture.h"

		SBackgroundSimpleScallingTexture() :
			vSize( VNULL2 )
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
	enum EPositionAllign;

	struct UI_EXPORT SBackgroundSimpleTexture : public SBackground
	{
		OBJECT_BASIC_METHODS( SBackgroundSimpleTexture )
	public:
		enum { typeID = 0x1106F441 };
		EPositionAllign eTextureX;
		EPositionAllign eTextureY;

		#include "include_backgroundsimpletexture.h"

		SBackgroundSimpleTexture() :
			eTextureX( EPA_LOW_END ),
			eTextureY( EPA_LOW_END )
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

	struct SSubRect
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CTPoint<float> ptSize;
		CTRect<float> rcMaps;
		CTRect<float> rcRect;
		int nRotate;

		SSubRect() :
			__dwCheckSum( 0 ),
			nRotate( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBackgroundTiledTexture : public SBackground
	{
		OBJECT_BASIC_METHODS( SBackgroundTiledTexture )
	public:
		enum { typeID = 0x1106CB02 };
		SSubRect rLT;
		SSubRect rRT;
		SSubRect rLB;
		SSubRect rRB;
		SSubRect rT;
		SSubRect rL;
		SSubRect rR;
		SSubRect rB;
		SSubRect rF;

		#include "include_backgroundtiledtexture.h"

		SBackgroundTiledTexture() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SWindowBaseShared;

	struct SWindowBaseShared : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		SWindowBaseShared() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct UI_EXPORT SWindowBaseDesc : public SUIDesc
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWindowBaseShared > pShared;

		SWindowBaseDesc() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
	struct SBackground;
	struct SUIDesc;

	struct SGameMessageReaction
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szGameMessage;
		string szLogicalReaction;
		SUIStateSequence visualReaction;
		bool bWaitVisual;
		bool bForward;

		SGameMessageReaction() :
			__dwCheckSum( 0 ),
			bWaitVisual( true ),
			bForward( true )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowPlacement
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CParam<CVec2> position;
		CParam<EPositionAllign> verAllign;
		CParam<EPositionAllign> horAllign;
		CParam<CVec2> size;
		CParam<CVec2> lowerMargin;
		CParam<CVec2> upperMargin;

		SWindowPlacement() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowFlags
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		bool bTransparent;

		SWindowFlags() :
			__dwCheckSum( 0 ),
			bTransparent( false )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct UI_EXPORT SWindowShared : public SWindowBaseShared
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SUIDesc > > children;
		CDBPtr< SBackground > pBackground;
		CDBPtr< SBackground > pForeground;
		SWindowFlags flags;
		SWindowPlacement placement;
		vector< CVec2 > activeArea;
		bool bIgnoreDblClick;

		SWindowShared() :
			__dwCheckSum( 0 ),
			bIgnoreDblClick( false )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct UI_EXPORT SWindow : public SWindowBaseDesc
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		NFile::CFilePath szTooltipFileRef;
		bool bVisible;
		int nPriority;
		vector< SGameMessageReaction > gameMessageReactions;
		SWindowPlacement placement;
		bool bEnabled;
		CDBPtr< SForegroundTextString > pTextString;

		#include "include_Window.h"

		SWindow() :
			__dwCheckSum( 0 ),
			bVisible( true ),
			nPriority( 0 ),
			bEnabled( true )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
	struct SForegroundTextStringShared;

	struct SForegroundTextStringShared : public CResource
	{
		OBJECT_BASIC_METHODS( SForegroundTextStringShared )
	public:
		enum { typeID = 0x1106CB40 };
		NFile::CFilePath szTextStringFileRef;
		SWindowPlacement position;
		NFile::CFilePath szFormatStringFileRef;

		#include "include_ForegroundTextStringShared.h"

		SForegroundTextStringShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SForegroundTextString : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SForegroundTextString )
	public:
		enum { typeID = 0x1106CB42 };
		CDBPtr< SForegroundTextStringShared > pShared;
		NFile::CFilePath szTextStringFileRef;

		#include "include_foregroundtextstring.h"

		SForegroundTextString() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct STextFormat : public SUIDesc
	{
		OBJECT_BASIC_METHODS( STextFormat )
	public:
		enum { typeID = 0x17159C40 };
		SWindowPlacement placement;
		NFile::CFilePath szFormatStringFileRef;

		#include "include_textformat.h"

		STextFormat() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowSimpleShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowSimpleShared )
	public:
		enum { typeID = 0x11082C40 };

		SWindowSimpleShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowSimple : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowSimple )
	public:
		enum { typeID = 0x1107C380 };

		#include "include_windowsimple.h"

		SWindowSimple() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SMessageSequence
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SUIDesc > > data;

		SMessageSequence() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMessageSequienceEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nCustomCheckReturn;
		SMessageSequence sequience;

		SMessageSequienceEntry() :
			__dwCheckSum( 0 ),
			nCustomCheckReturn( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SCheckRunScript : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SCheckRunScript )
	public:
		enum { typeID = 0x1106BC41 };
		string szScriptFunction;

		#include "include_checkrunscript.h"

		SCheckRunScript() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SCheckPreprogrammed : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SCheckPreprogrammed )
	public:
		enum { typeID = 0x1106BC42 };
		string szCheckName;

		#include "include_checkpreprogrammed.h"

		SCheckPreprogrammed() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SCheckIsWindowEnabled : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SCheckIsWindowEnabled )
	public:
		enum { typeID = 0x15083380 };
		string szWindowName;
		string szParentWindowName;

		#include "include_checkiswindowenabled.h"

		SCheckIsWindowEnabled() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SCheckIsWindowVisible : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SCheckIsWindowVisible )
	public:
		enum { typeID = 0x110B3400 };
		string szWindowName;
		string szParentWindowName;

		#include "include_checkiswindowvisible.h"

		SCheckIsWindowVisible() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SCheckIsTabActive : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SCheckIsTabActive )
	public:
		enum { typeID = 0x170B6300 };
		string szTabControlName;
		int nTab;

		#include "include_checkistabactive.h"

		SCheckIsTabActive() :
			nTab( 0 )
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

	struct SMessageReactionComplex : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SMessageReactionComplex )
	public:
		enum { typeID = 0x1106BC43 };
		vector< SMessageSequienceEntry > branches;
		CDBPtr< SUIDesc > pConditionCheck;
		SMessageSequence commonBefore;
		SMessageSequence commonAfter;

		#include "include_messagereactioncomplex.h"

		SMessageReactionComplex() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SARSetGlobalVar : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSetGlobalVar )
	public:
		enum { typeID = 0x1106BC44 };
		string szVarName;
		string szVarValue;

		#include "include_arsetglobalvar.h"

		SARSetGlobalVar() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SARRemoveGlobalVar : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARRemoveGlobalVar )
	public:
		enum { typeID = 0x1106BC45 };
		string szVarName;

		#include "include_arremoveglobalvar.h"

		SARRemoveGlobalVar() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SARSendUIMessage : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSendUIMessage )
	public:
		enum { typeID = 0x1106BC46 };
		string szMessageID;
		string szStringParam;
		int nIntParam;

		#include "include_arsenduimessage.h"

		SARSendUIMessage() :
			nIntParam( 0 )
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

	struct SARSendGameMessage : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSendGameMessage )
	public:
		enum { typeID = 0x15084340 };
		string szEventName;
		int nIntParam;

		#include "include_arsendgamemessage.h"

		SARSendGameMessage() :
			nIntParam( 0 )
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

	struct SARSwitchTab : public SUIDesc
	{
		OBJECT_BASIC_METHODS( SARSwitchTab )
	public:
		enum { typeID = 0x15083384 };
		string szTabControlName;
		int nTab;

		#include "include_arswitchtab.h"

		SARSwitchTab() :
			nTab( 0 )
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

	struct SReactionSequenceEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		CDBPtr< SUIDesc > pReaction;

		SReactionSequenceEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMessageReactionsDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SReactionSequenceEntry > reactions;
		NFile::CFilePath szScriptFileRef;

		SMessageReactionsDesc() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SCommandSequienceEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		SUIStateSequence sequence;

		SCommandSequienceEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScreenTextEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		NFile::CFilePath szTextFileRef;

		SScreenTextEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowScreenShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowScreenShared )
	public:
		enum { typeID = 0x1106BC48 };

		SWindowScreenShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowScreen : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowScreen )
	public:
		enum { typeID = 0x1106BC4A };
		SMessageReactionsDesc messageReactions;
		vector< SCommandSequienceEntry > commandSequiences;
		int nTooltipContext;
		vector< SScreenTextEntry > relatedTexts;

		#include "include_screen.h"

		SWindowScreen() :
			nTooltipContext( 0 )
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
	struct SBackground;

	struct SWindowProgressBarShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowProgressBarShared )
	public:
		enum { typeID = 0x1106C405 };
		CDBPtr< SBackground > pForward;
		CDBPtr< SBackground > pBackward;
		CDBPtr< SBackground > pGlow;
		float fStepSize;
		CVec2 vGlowSize;

		SWindowProgressBarShared() :
			fStepSize( 0.0f ),
			vGlowSize( VNULL2 )
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

	struct SWindowProgressBar : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowProgressBar )
	public:
		enum { typeID = 0x1106C402 };
		float fProgress;

		#include "include_windowprogressbar.h"

		SWindowProgressBar() :
			fProgress( 0.0f )
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

	struct SProgressBarTextureInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nMaxValue;
		CDBPtr< SBackground > pTexture;

		SProgressBarTextureInfo() :
			__dwCheckSum( 0 ),
			nMaxValue( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMultiTextureProgressBarSharedState
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fValue;
		CDBPtr< SBackground > pBackground;

		SMultiTextureProgressBarSharedState() :
			__dwCheckSum( 0 ),
			fValue( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowMultiTextureProgressBarShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowMultiTextureProgressBarShared )
	public:
		enum { typeID = 0x150A0AC1 };
		vector< SMultiTextureProgressBarSharedState > states;

		SWindowMultiTextureProgressBarShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowMultiTextureProgressBar : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowMultiTextureProgressBar )
	public:
		enum { typeID = 0x150A0AC2 };
		vector< float > progresses;
		bool bSolid;
		float fPieceSize;

		#include "include_windowmultitextureprogressbar.h"

		SWindowMultiTextureProgressBar() :
			bSolid( true ),
			fPieceSize( 0.0f )
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
	struct STextFormat;

	struct SWindowTextViewShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowTextViewShared )
	public:
		enum { typeID = 0x1106C3C2 };
		int nColor;
		int nFormat;
		string szFontName;
		int nRedLineSpace;

		SWindowTextViewShared() :
			nColor( 0 ),
			nFormat( 0 ),
			nRedLineSpace( 0 )
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

	struct SWindowTextView : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowTextView )
	public:
		enum { typeID = 0x1106C3C4 };
		NFile::CFilePath szTextFileRef;
		bool bResizeOnTextSet;
		CDBPtr< STextFormat > pTextFormat;

		#include "include_windowtextview.h"

		SWindowTextView() :
			bResizeOnTextSet( true )
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

	struct SWindowTooltipShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowTooltipShared )
	public:
		enum { typeID = 0x1106C409 };
		CVec2 vLowerBorder;
		CVec2 vHigherBorder;

		SWindowTooltipShared() :
			vLowerBorder( VNULL2 ),
			vHigherBorder( VNULL2 )
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

	struct SWindowTooltip : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowTooltip )
	public:
		enum { typeID = 0x1106C40A };

		#include "include_windowtooltip.h"

		SWindowTooltip() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowPlayerShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowPlayerShared )
	public:
		enum { typeID = 0x170A7B80 };

		SWindowPlayerShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowPlayer : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowPlayer )
	public:
		enum { typeID = 0x170A7B81 };
		string szSequenceName;
		bool bMaintainAspectRatio;

		#include "include_windowplayer.h"

		SWindowPlayer() :
			bMaintainAspectRatio( true )
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
	enum ETextEntryType;

	enum ETextEntryType
	{
		ETET_ALL = 0,
		ETET_NUMERIC = 1,
		ETET_GAME_SPY = 2,
		ETET_LOCAL_PLAYERNAME = 3,
		ETET_FILENAME = 4,
	};

	struct SWindowEditLineShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowEditLineShared )
	public:
		enum { typeID = 0x1106C340 };
		string szFontName;
		int nColor;
		int nCursorColor;
		int nSelColor;
		int nLeftSpace;
		int nRightSpace;
		int nYOffset;

		SWindowEditLineShared() :
			nColor( 0 ),
			nCursorColor( 0 ),
			nSelColor( 0 ),
			nLeftSpace( 0 ),
			nRightSpace( 0 ),
			nYOffset( 0 )
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

	struct SWindowEditLine : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowEditLine )
	public:
		enum { typeID = 0x1106C342 };
		string szOnReturn;
		SUIStateSequence sequienceOnReturn;
		string szOnEscape;
		SUIStateSequence sequienceOnEscape;
		int nMaxLength;
		bool bTextScroll;
		ETextEntryType eTextEntryType;
		bool bPassword;
		SUIStateSequence sequienceOnTextChanged;
		string szOnTextChanged;
		int nTabOrder;
		SUIStateSequence sequienceOnFocusLost;

		#include "include_windoweditline.h"

		SWindowEditLine() :
			nMaxLength( -1 ),
			bTextScroll( false ),
			eTextEntryType( ETET_ALL ),
			bPassword( false ),
			nTabOrder( -1 )
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

	struct SWindowConsoleOutputShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowConsoleOutputShared )
	public:
		enum { typeID = 0x11095B02 };
		bool bAutoDelete;

		SWindowConsoleOutputShared() :
			bAutoDelete( false )
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

	struct SWindowConsoleOutput : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowConsoleOutput )
	public:
		enum { typeID = 0x11095B01 };

		#include "include_windowconsoleoutput.h"

		SWindowConsoleOutput() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowStatsSystemShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowStatsSystemShared )
	public:
		enum { typeID = 0x110AC480 };

		SWindowStatsSystemShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowStatsSystem : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowStatsSystem )
	public:
		enum { typeID = 0x110AC4C0 };

		#include "include_windowstatssystem.h"

		SWindowStatsSystem() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowConsoleShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowConsoleShared )
	public:
		enum { typeID = 0x1106C303 };
		int nColor;
		CDBPtr< SWindowEditLine > pEditLine;
		SUIStateSequence makeVisible;

		SWindowConsoleShared() :
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

	struct SWindowConsole : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowConsole )
	public:
		enum { typeID = 0x1106C304 };

		#include "include_windowconsole.h"

		SWindowConsole() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SWindowScrollBar;
	struct SWindow;

	struct SWindowScrollableContainerBaseShared : public SWindowShared
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nInterval;
		CDBPtr< SWindowScrollBar > pScrollBar;
		CDBPtr< SWindow > pBorder;
		CDBPtr< SWindow > pSelection;
		CDBPtr< SWindow > pPreSelection;
		CDBPtr< SWindow > pNegativeSelection;

		SWindowScrollableContainerBaseShared() :
			__dwCheckSum( 0 ),
			nInterval( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowScrollableContainerBase : public SWindow
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		SUIStateSequence onSelection;
		SUIStateSequence onDoubleClick;

		SWindowScrollableContainerBase() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowScrollableContainerShared : public SWindowScrollableContainerBaseShared
	{
		OBJECT_BASIC_METHODS( SWindowScrollableContainerShared )
	public:
		enum { typeID = 0x170AF300 };

		SWindowScrollableContainerShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowScrollableContainer : public SWindowScrollableContainerBase
	{
		OBJECT_BASIC_METHODS( SWindowScrollableContainer )
	public:
		enum { typeID = 0x1107C381 };

		#include "include_windowscrollablecontainer.h"

		SWindowScrollableContainer() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindow1LvlTreeControlShared : public SWindowScrollableContainerBaseShared
	{
		OBJECT_BASIC_METHODS( SWindow1LvlTreeControlShared )
	public:
		enum { typeID = 0x1106C3C0 };
		CDBPtr< SWindow > pItemSample;
		CDBPtr< SWindow > pSubItemSample;

		SWindow1LvlTreeControlShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindow1LvlTreeControl : public SWindowScrollableContainerBase
	{
		OBJECT_BASIC_METHODS( SWindow1LvlTreeControl )
	public:
		enum { typeID = 0x1106C3C1 };

		#include "include_window1lvltreecontrol.h"

		SWindow1LvlTreeControl() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SWindowListHeader;
	struct SWindowListItem;

	struct SWindowListHeaderShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowListHeaderShared )
	public:
		enum { typeID = 0x1106C2C0 };
		CDBPtr< SWindow > pSortIconDown;
		CDBPtr< SWindow > pSortIconUp;
		vector< CDBPtr< SWindowMSButton > > subHeaderSamples;

		SWindowListHeaderShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowListHeader : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowListHeader )
	public:
		enum { typeID = 0x1106C2C2 };

		#include "include_windowlistheader.h"

		SWindowListHeader() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowListItemShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowListItemShared )
	public:
		enum { typeID = 0x170CC480 };
		vector< CDBPtr< SWindow > > subItemSamples;

		SWindowListItemShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowListItem : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowListItem )
	public:
		enum { typeID = 0x1106C2C3 };

		#include "include_windowlistitem.h"

		SWindowListItem() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowListSharedData : public SWindowScrollableContainerBaseShared
	{
		OBJECT_BASIC_METHODS( SWindowListSharedData )
	public:
		enum { typeID = 0x1106C301 };
		CDBPtr< SWindowListItem > pListItem;
		CDBPtr< SWindowListHeader > pListHeader;

		SWindowListSharedData() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowListCtrl : public SWindowScrollableContainerBase
	{
		OBJECT_BASIC_METHODS( SWindowListCtrl )
	public:
		enum { typeID = 0x1106C302 };

		#include "include_windowlistctrl.h"

		SWindowListCtrl() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SWindowScrollableContainerBase;

	struct SWindowTabControlShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowTabControlShared )
	public:
		enum { typeID = 0x1106C440 };
		CDBPtr< SWindowScrollableContainerBase > pHeadersContainer;
		CDBPtr< SWindow > pContainerSample;
		CDBPtr< SWindowMSButton > pButtonSample;

		SWindowTabControlShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowTabControl : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowTabControl )
	public:
		enum { typeID = 0x1106C442 };

		struct STab
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SWindow > pTabContainer;
			string szButtonName;

			STab() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< STab > tabs;

		#include "include_windowtabcontrol.h"

		SWindowTabControl() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	struct SWindowListCtrl;
	struct SWindowMSButton;
	struct SWindow;

	struct SWindowComboBoxShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowComboBoxShared )
	public:
		enum { typeID = 0x17122380 };
		CDBPtr< SWindow > pLine;
		CDBPtr< SWindowMSButton > pIcon;
		CDBPtr< SWindowListCtrl > pList;

		SWindowComboBoxShared() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWindowComboBox : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowComboBox )
	public:
		enum { typeID = 0x17122381 };
		int nListPriority;
		int nMaxVisibleRows;
		SUIStateSequence onSelection;

		#include "include_windowcombobox.h"

		SWindowComboBox() :
			nListPriority( 0 ),
			nMaxVisibleRows( 0 )
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
	enum EButtonSubstateType;
	struct STextFormat;
	enum EButtonChangeStateType;

	enum EButtonSubstateType
	{
		BST_NORMAL = 0,
		BST_MOUSE_OVER = 1,
		BST_PUSHED_DEEP = 2,
		BST_DISABLED = 3,
		BST_RIGHT_DOWN = 4,
	};

	enum EButtonChangeStateType
	{
		BCST_ON_PUSH = 0,
		BCST_ON_RELEASE = 1,
	};

	struct SButtonVisualSubState
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SBackground > pBackground;
		CDBPtr< SBackground > pForeground;
		CDBPtr< SForegroundTextString > pTextString;
		SUIStateSequence onEnterSubState;
		CDBPtr< STextFormat > pTextFormat;

		SButtonVisualSubState() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SButtonVisualState
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		SButtonVisualSubState normal;
		SButtonVisualSubState mouseOver;
		SButtonVisualSubState pushed;
		SButtonVisualSubState disabled;
		SButtonVisualSubState rightButtonDown;
		EButtonSubstateType eDefaultSubState;
		SUIStateSequence visualOnEnterState;

		SButtonVisualState() :
			__dwCheckSum( 0 ),
			eDefaultSubState( BST_NORMAL )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SButtonLogicalState
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szMessageOnEnterState;
		SUIStateSequence commandsOnEnterState;
		SUIStateSequence commandsOnRightClick;
		SUIStateSequence commandsOnLDblKlick;
		bool bWaitVisual;
		bool bReverseCommands;
		string szName;

		SButtonLogicalState() :
			__dwCheckSum( 0 ),
			bWaitVisual( false ),
			bReverseCommands( false )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowMSButtonShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowMSButtonShared )
	public:
		enum { typeID = 0x1106C380 };
		vector< SButtonVisualState > visualStates;
		EButtonChangeStateType eTriggerMode;

		SWindowMSButtonShared() :
			eTriggerMode( BCST_ON_RELEASE )
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

	struct SWindowMSButton : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowMSButton )
	public:
		enum { typeID = 0x1106C382 };
		vector< SButtonLogicalState > buttonStates;
		int nButtonGroupID;
		bool bAutoChangeState;
		SUIStateSequence pushEffect;
		int nState;
		NFile::CFilePath szTextFileRef;
		CDBPtr< STextFormat > pTextFormat;

		#include "include_windowmsbutton.h"

		SWindowMSButton() :
			nButtonGroupID( 0 ),
			bAutoChangeState( false ),
			nState( 0 )
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

	struct SWindowSliderShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowSliderShared )
	public:
		enum { typeID = 0x1106C401 };
		bool bHorisontal;
		CDBPtr< SWindowMSButton > pLever;
		float fMinLeverSize;
		float fMaxLeverSize;

		SWindowSliderShared() :
			bHorisontal( false ),
			fMinLeverSize( 32 ),
			fMaxLeverSize( 0 )
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

	struct SWindowSlider : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowSlider )
	public:
		enum { typeID = 0x1106C400 };
		int nSpecialPositions;

		#include "include_windowslider.h"

		SWindowSlider() :
			nSpecialPositions( 0 )
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
	struct SWindowSlider;

	struct SWindowScrollBarShared : public SWindowShared
	{
		OBJECT_BASIC_METHODS( SWindowScrollBarShared )
	public:
		enum { typeID = 0x1106C383 };
		float fSpeed;
		CDBPtr< SWindowMSButton > pButtonLower;
		CDBPtr< SWindowMSButton > pButtonGreater;
		CDBPtr< SWindowSlider > pSlider;

		SWindowScrollBarShared() :
			fSpeed( 0.0f )
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

	struct SWindowScrollBar : public SWindow
	{
		OBJECT_BASIC_METHODS( SWindowScrollBar )
	public:
		enum { typeID = 0x1106C384 };
		SUIStateSequence effects;

		#include "include_windowscrollbar.h"

		SWindowScrollBar() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
	enum EButtonSubstateType;

	struct SUISButtonSubstate : public SUIStateBase
	{
		OBJECT_BASIC_METHODS( SUISButtonSubstate )
	public:
		enum { typeID = 0x170AE340 };
		EButtonSubstateType eSubstate;
		float fWaitTime;
		CParam<string> szButton;

		#include "include_uisbuttonsubstate.h"

		SUISButtonSubstate() :
			eSubstate( BST_NORMAL ),
			fWaitTime( 0.0f )
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
	string EnumToString( NDb::EPositionAllign eValue );
	EPositionAllign StringToEnum_NDb_EPositionAllign( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EPositionAllign>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EPositionAllign eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EPositionAllign ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EPositionAllign( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::ETextEntryType eValue );
	ETextEntryType StringToEnum_NDb_ETextEntryType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::ETextEntryType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::ETextEntryType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ETextEntryType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_ETextEntryType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EButtonSubstateType eValue );
	EButtonSubstateType StringToEnum_NDb_EButtonSubstateType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EButtonSubstateType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EButtonSubstateType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EButtonSubstateType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EButtonSubstateType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EButtonChangeStateType eValue );
	EButtonChangeStateType StringToEnum_NDb_EButtonChangeStateType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EButtonChangeStateType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EButtonChangeStateType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EButtonChangeStateType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EButtonChangeStateType( szValue ); }
};
