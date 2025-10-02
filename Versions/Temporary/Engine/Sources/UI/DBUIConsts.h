#pragma once

#include "UI_export.h"


// automatically generated file, don't change manually!

#include "dbuserinterface.h"

interface IXmlSaver;

namespace NDb
{
	struct SWindowTooltip;
	struct SUIStateBase;
	struct STooltipContext;
	struct SWindowConsole;
	struct SWindowStatsSystem;
	struct SWindowSimple;

	struct STooltipContext : public CResource
	{
		OBJECT_BASIC_METHODS( STooltipContext )
	public:
		enum { typeID = 0x15087B80 };
		CDBPtr< SWindowTooltip > pWindow;
		vector< CDBPtr< SUIStateBase > > appearCommands;
		int nMouseMaxOffsetToAppear;
		int nAppearDelay;
		int nSingleLineWidth;
		float fHorisontalToVerticalRatio;

		STooltipContext() :
			nMouseMaxOffsetToAppear( 0 ),
			nAppearDelay( 0 ),
			nSingleLineWidth( 0 ),
			fHorisontalToVerticalRatio( 0.0f )
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

	struct UI_EXPORT SUIGameConsts : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< STooltipContext > > contexts;
		CDBPtr< SWindowConsole > pConsole;
		CDBPtr< SWindowSimple > pDebugInfo;
		CDBPtr< SWindowStatsSystem > pStatsWindow;
		SUIStateSequence buttonClickSound;

		SUIGameConsts() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}
