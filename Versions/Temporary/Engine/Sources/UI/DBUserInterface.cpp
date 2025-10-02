// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbuserinterface.h"

namespace NDb
{


string EnumToString( NDb::EPositionAllign eValue )
{
	switch ( eValue )
	{
	case NDb::EPA_LOW_END:
		return "EPA_LOW_END";
	case NDb::ERA_CENTER:
		return "ERA_CENTER";
	case NDb::EPA_HIGH_END:
		return "EPA_HIGH_END";
	case NDb::EPA_MARGIN:
		return "EPA_MARGIN";
	default:
		return "EPA_LOW_END";
	}
}

NDb::EPositionAllign NDb::StringToEnum_NDb_EPositionAllign( const string &szValue )
{
	if ( szValue == "EPA_LOW_END" )
		return NDb::EPA_LOW_END;
	if ( szValue == "ERA_CENTER" )
		return NDb::ERA_CENTER;
	if ( szValue == "EPA_HIGH_END" )
		return NDb::EPA_HIGH_END;
	if ( szValue == "EPA_MARGIN" )
		return NDb::EPA_MARGIN;
	return NDb::EPA_LOW_END;
}


void SUIDesc::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ClassTypeID", (BYTE*)&nClassTypeID - pThis, sizeof(nClassTypeID), NTypeDef::TYPE_TYPE_INT );
}

int SUIDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "ClassTypeID", &nClassTypeID );

	return 0;
}

int SUIDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nClassTypeID );

	return 0;
}

DWORD SUIDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nClassTypeID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUICommandBase::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "szParam1", &szParam1, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "szParam2", &szParam2, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "vParam1", &vParam1, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "nParam1", &nParam1, pThis ); 
}

int SUICommandBase::operator&( IXmlSaver &saver )
{
	saver.Add( "szParam1", &szParam1 );
	saver.Add( "szParam2", &szParam2 );
	saver.Add( "vParam1", &vParam1 );
	saver.Add( "nParam1", &nParam1 );

	return 0;
}

int SUICommandBase::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szParam1 );
	saver.Add( 3, &szParam2 );
	saver.Add( 4, &vParam1 );
	saver.Add( 5, &nParam1 );

	return 0;
}

DWORD SUICommandBase::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szParam1 << szParam2 << vParam1 << nParam1;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBUIMessage::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MessageID", (BYTE*)&szMessageID - pThis, sizeof(szMessageID), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "StringParam", (BYTE*)&szStringParam - pThis, sizeof(szStringParam), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "IntParam", (BYTE*)&nIntParam - pThis, sizeof(nIntParam), NTypeDef::TYPE_TYPE_INT );
}

int SBUIMessage::operator&( IXmlSaver &saver )
{
	saver.Add( "MessageID", &szMessageID );
	saver.Add( "StringParam", &szStringParam );
	saver.Add( "IntParam", &nIntParam );

	return 0;
}

int SBUIMessage::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szMessageID );
	saver.Add( 3, &szStringParam );
	saver.Add( 4, &nIntParam );

	return 0;
}

DWORD SBUIMessage::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szMessageID << szStringParam << nIntParam;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUIStateBaseShared::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
}

int SUIStateBaseShared::operator&( IXmlSaver &saver )
{

	return 0;
}

int SUIStateBaseShared::operator&( IBinSaver &saver )
{

	return 0;
}

DWORD SUIStateBaseShared::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUIStateBase::ReportMetaInfo() const
{
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "pShared", (BYTE*)&ppShared - pThis, sizeof(ppShared), NTypeDef::TYPE_TYPE_REF );
}

int SUIStateBase::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "pShared", &ppShared );

	return 0;
}

int SUIStateBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &ppShared );

	return 0;
}

DWORD SUIStateBase::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SUIDesc::CalcCheckSum() << ppShared;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUISMoveTo::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISMoveTo", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "vOffset", &vOffset, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "fMoveTime", &fMoveTime, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "szElementToMove", &szElementToMove, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISMoveTo::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "vOffset", &vOffset );
	saver.Add( "fMoveTime", &fMoveTime );
	saver.Add( "szElementToMove", &szElementToMove );

	return 0;
}

int SUISMoveTo::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &vOffset );
	saver.Add( 3, &fMoveTime );
	saver.Add( 4, &szElementToMove );

	return 0;
}



void SUISRunReaction::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISRunReaction", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "szReactionForward", &szReactionForward, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "szReactionBack", &szReactionBack, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISRunReaction::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "szReactionForward", &szReactionForward );
	saver.Add( "szReactionBack", &szReactionBack );

	return 0;
}

int SUISRunReaction::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &szReactionForward );
	saver.Add( 3, &szReactionBack );

	return 0;
}



void SUISSendUIMessage::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISSendUIMessage", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "szMessageID", &szMessageID, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "szParam", &szParam, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "nForwardParam", &nForwardParam, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "nBackParam", &nBackParam, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISSendUIMessage::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "szMessageID", &szMessageID );
	saver.Add( "szParam", &szParam );
	saver.Add( "nForwardParam", &nForwardParam );
	saver.Add( "nBackParam", &nBackParam );

	return 0;
}

int SUISSendUIMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &szMessageID );
	saver.Add( 3, &szParam );
	saver.Add( 4, &nForwardParam );
	saver.Add( 5, &nBackParam );

	return 0;
}



void SUIConsoleCommand::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UIConsoleCommand", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "EditBoxName", (BYTE*)&szEditBoxName - pThis, sizeof(szEditBoxName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SUIConsoleCommand::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "EditBoxName", &szEditBoxName );

	return 0;
}

int SUIConsoleCommand::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &szEditBoxName );

	return 0;
}



void SUISDirectRunReaction::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISDirectRunReaction", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ReactionForward", (BYTE*)&pReactionForward - pThis, sizeof(pReactionForward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ReactionBackward", (BYTE*)&pReactionBackward - pThis, sizeof(pReactionBackward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SUISDirectRunReaction::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "ReactionForward", &pReactionForward );
	saver.Add( "ReactionBackward", &pReactionBackward );

	return 0;
}

int SUISDirectRunReaction::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &pReactionForward );
	saver.Add( 3, &pReactionBackward );

	return 0;
}



void SUIStateSequence::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Commands", &commands, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Reversable", (BYTE*)&bReversable - pThis, sizeof(bReversable), NTypeDef::TYPE_TYPE_BOOL );
}

int SUIStateSequence::operator&( IXmlSaver &saver )
{
	saver.Add( "Commands", &commands );
	saver.Add( "Reversable", &bReversable );

	return 0;
}

int SUIStateSequence::operator&( IBinSaver &saver )
{
	saver.Add( 2, &commands );
	saver.Add( 3, &bReversable );

	return 0;
}

DWORD SUIStateSequence::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << commands << bReversable;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBackground::ReportMetaInfo() const
{
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
}

int SBackground::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "Texture", &pTexture );
	saver.Add( "Color", &nColor );

	return 0;
}

int SBackground::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &pTexture );
	saver.Add( 3, &nColor );

	return 0;
}

DWORD SBackground::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SUIDesc::CalcCheckSum() << nColor;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBackgroundSimpleScallingTexture::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BackgroundSimpleScallingTexture", typeID, sizeof(*this) );
	SBackground::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Size", &vSize, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SBackgroundSimpleScallingTexture::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SBackground*)(this) );
	saver.Add( "Size", &vSize );

	return 0;
}

int SBackgroundSimpleScallingTexture::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBackground*)this );
	saver.Add( 2, &vSize );

	return 0;
}



void SBackgroundSimpleTexture::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BackgroundSimpleTexture", typeID, sizeof(*this) );
	SBackground::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TextureX", (BYTE*)&eTextureX - pThis, sizeof(eTextureX), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "TextureY", (BYTE*)&eTextureY - pThis, sizeof(eTextureY), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SBackgroundSimpleTexture::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SBackground*)(this) );
	saver.Add( "TextureX", &eTextureX );
	saver.Add( "TextureY", &eTextureY );

	return 0;
}

int SBackgroundSimpleTexture::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBackground*)this );
	saver.Add( 2, &eTextureX );
	saver.Add( 3, &eTextureY );

	return 0;
}



void SSubRect::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Size", &ptSize, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Maps", &rcMaps, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Rect", &rcRect, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Rotate", (BYTE*)&nRotate - pThis, sizeof(nRotate), NTypeDef::TYPE_TYPE_INT );
}

int SSubRect::operator&( IXmlSaver &saver )
{
	saver.Add( "Size", &ptSize );
	saver.Add( "Maps", &rcMaps );
	saver.Add( "Rect", &rcRect );
	saver.Add( "Rotate", &nRotate );

	return 0;
}

int SSubRect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &ptSize );
	saver.Add( 3, &rcMaps );
	saver.Add( 4, &rcRect );
	saver.Add( 5, &nRotate );

	return 0;
}

DWORD SSubRect::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << ptSize << rcMaps << rcRect << nRotate;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBackgroundTiledTexture::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BackgroundTiledTexture", typeID, sizeof(*this) );
	SBackground::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "rLT", &rLT, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rRT", &rRT, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rLB", &rLB, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rRB", &rRB, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rT", &rT, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rL", &rL, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rR", &rR, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rB", &rB, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rF", &rF, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SBackgroundTiledTexture::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SBackground*)(this) );
	saver.Add( "rLT", &rLT );
	saver.Add( "rRT", &rRT );
	saver.Add( "rLB", &rLB );
	saver.Add( "rRB", &rRB );
	saver.Add( "rT", &rT );
	saver.Add( "rL", &rL );
	saver.Add( "rR", &rR );
	saver.Add( "rB", &rB );
	saver.Add( "rF", &rF );

	return 0;
}

int SBackgroundTiledTexture::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBackground*)this );
	saver.Add( 2, &rLT );
	saver.Add( 3, &rRT );
	saver.Add( 4, &rLB );
	saver.Add( 5, &rRB );
	saver.Add( 6, &rT );
	saver.Add( 7, &rL );
	saver.Add( 8, &rR );
	saver.Add( 9, &rB );
	saver.Add( 10, &rF );

	return 0;
}



void SWindowBaseShared::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
}

int SWindowBaseShared::operator&( IXmlSaver &saver )
{

	return 0;
}

int SWindowBaseShared::operator&( IBinSaver &saver )
{

	return 0;
}

DWORD SWindowBaseShared::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowBaseDesc::ReportMetaInfo() const
{
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Shared", (BYTE*)&pShared - pThis, sizeof(pShared), NTypeDef::TYPE_TYPE_REF );
}

int SWindowBaseDesc::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "Shared", &pShared );

	return 0;
}

int SWindowBaseDesc::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &pShared );

	return 0;
}

DWORD SWindowBaseDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SUIDesc::CalcCheckSum() << pShared;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SGameMessageReaction::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "GameMessage", (BYTE*)&szGameMessage - pThis, sizeof(szGameMessage), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "LogicalReaction", (BYTE*)&szLogicalReaction - pThis, sizeof(szLogicalReaction), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "VisualReaction", &visualReaction, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "WaitVisual", (BYTE*)&bWaitVisual - pThis, sizeof(bWaitVisual), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Forward", (BYTE*)&bForward - pThis, sizeof(bForward), NTypeDef::TYPE_TYPE_BOOL );
}

int SGameMessageReaction::operator&( IXmlSaver &saver )
{
	saver.Add( "GameMessage", &szGameMessage );
	saver.Add( "LogicalReaction", &szLogicalReaction );
	saver.Add( "VisualReaction", &visualReaction );
	saver.Add( "WaitVisual", &bWaitVisual );
	saver.Add( "Forward", &bForward );

	return 0;
}

int SGameMessageReaction::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szGameMessage );
	saver.Add( 3, &szLogicalReaction );
	saver.Add( 4, &visualReaction );
	saver.Add( 5, &bWaitVisual );
	saver.Add( 6, &bForward );

	return 0;
}

DWORD SGameMessageReaction::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szGameMessage << szLogicalReaction << visualReaction << bWaitVisual << bForward;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowPlacement::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &position, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "VerAllign", &verAllign, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "HorAllign", &horAllign, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Size", &size, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "LowerMargin", &lowerMargin, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "UpperMargin", &upperMargin, pThis ); 
}

int SWindowPlacement::operator&( IXmlSaver &saver )
{
	saver.Add( "Position", &position );
	saver.Add( "VerAllign", &verAllign );
	saver.Add( "HorAllign", &horAllign );
	saver.Add( "Size", &size );
	saver.Add( "LowerMargin", &lowerMargin );
	saver.Add( "UpperMargin", &upperMargin );

	return 0;
}

int SWindowPlacement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &position );
	saver.Add( 3, &verAllign );
	saver.Add( 4, &horAllign );
	saver.Add( 5, &size );
	saver.Add( 6, &lowerMargin );
	saver.Add( 7, &upperMargin );

	return 0;
}

DWORD SWindowPlacement::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << position << verAllign << horAllign << size << lowerMargin << upperMargin;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowFlags::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Transparent", (BYTE*)&bTransparent - pThis, sizeof(bTransparent), NTypeDef::TYPE_TYPE_BOOL );
}

int SWindowFlags::operator&( IXmlSaver &saver )
{
	saver.Add( "Transparent", &bTransparent );

	return 0;
}

int SWindowFlags::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bTransparent );

	return 0;
}

DWORD SWindowFlags::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << bTransparent;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowShared::ReportMetaInfo() const
{
	SWindowBaseShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Children", &children, pThis );
	NMetaInfo::ReportMetaInfo( "Background", (BYTE*)&pBackground - pThis, sizeof(pBackground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Foreground", (BYTE*)&pForeground - pThis, sizeof(pForeground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "Flags", &flags, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Placement", &placement, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "ActiveArea", &activeArea, pThis );
	NMetaInfo::ReportMetaInfo( "IgnoreDblClick", (BYTE*)&bIgnoreDblClick - pThis, sizeof(bIgnoreDblClick), NTypeDef::TYPE_TYPE_BOOL );
}

int SWindowShared::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SWindowBaseShared*)(this) );
	saver.Add( "Children", &children );
	saver.Add( "Background", &pBackground );
	saver.Add( "Foreground", &pForeground );
	saver.Add( "Flags", &flags );
	saver.Add( "Placement", &placement );
	saver.Add( "ActiveArea", &activeArea );
	saver.Add( "IgnoreDblClick", &bIgnoreDblClick );

	return 0;
}

int SWindowShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowBaseShared*)this );
	saver.Add( 2, &children );
	saver.Add( 3, &pBackground );
	saver.Add( 4, &pForeground );
	saver.Add( 5, &flags );
	saver.Add( 6, &placement );
	saver.Add( 7, &activeArea );
	saver.Add( 8, &bIgnoreDblClick );

	return 0;
}

DWORD SWindowShared::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SWindowBaseShared::CalcCheckSum() << children << pBackground << pForeground << flags << placement << activeArea << bIgnoreDblClick;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindow::ReportMetaInfo() const
{
	SWindowBaseDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Visible", (BYTE*)&bVisible - pThis, sizeof(bVisible), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "GameMessageReactions", &gameMessageReactions, pThis );
	NMetaInfo::ReportStructMetaInfo( "Placement", &placement, pThis ); 
	NMetaInfo::ReportMetaInfo( "Enabled", (BYTE*)&bEnabled - pThis, sizeof(bEnabled), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "TextString", (BYTE*)&pTextString - pThis, sizeof(pTextString), NTypeDef::TYPE_TYPE_REF );
}

int SWindow::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SWindowBaseDesc*)(this) );
	saver.Add( "Name", &szName );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Visible", &bVisible );
	saver.Add( "Priority", &nPriority );
	saver.Add( "GameMessageReactions", &gameMessageReactions );
	saver.Add( "Placement", &placement );
	saver.Add( "Enabled", &bEnabled );
	saver.Add( "TextString", &pTextString );

	return 0;
}

int SWindow::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowBaseDesc*)this );
	saver.Add( 2, &szName );
	saver.Add( 3, &szTooltipFileRef );
	saver.Add( 4, &bVisible );
	saver.Add( 5, &nPriority );
	saver.Add( 6, &gameMessageReactions );
	saver.Add( 7, &placement );
	saver.Add( 8, &bEnabled );
	saver.Add( 9, &pTextString );

	return 0;
}

DWORD SWindow::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SWindowBaseDesc::CalcCheckSum() << szName << bVisible << nPriority << gameMessageReactions << placement << bEnabled;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SForegroundTextStringShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ForegroundTextStringShared", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TextStringFileRef", (BYTE*)&szTextStringFileRef - pThis, sizeof(szTextStringFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "Position", &position, pThis ); 
	NMetaInfo::ReportMetaInfo( "FormatStringFileRef", (BYTE*)&szFormatStringFileRef - pThis, sizeof(szFormatStringFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SForegroundTextStringShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TextStringFileRef", &szTextStringFileRef );
	saver.Add( "Position", &position );
	saver.Add( "FormatStringFileRef", &szFormatStringFileRef );

	return 0;
}

int SForegroundTextStringShared::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szTextStringFileRef );
	saver.Add( 3, &position );
	saver.Add( 4, &szFormatStringFileRef );

	return 0;
}



void SForegroundTextString::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ForegroundTextString", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Shared", (BYTE*)&pShared - pThis, sizeof(pShared), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TextStringFileRef", (BYTE*)&szTextStringFileRef - pThis, sizeof(szTextStringFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SForegroundTextString::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "Shared", &pShared );
	saver.Add( "TextStringFileRef", &szTextStringFileRef );

	return 0;
}

int SForegroundTextString::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &pShared );
	saver.Add( 3, &szTextStringFileRef );

	return 0;
}



void STextFormat::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TextFormat", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Placement", &placement, pThis ); 
	NMetaInfo::ReportMetaInfo( "FormatStringFileRef", (BYTE*)&szFormatStringFileRef - pThis, sizeof(szFormatStringFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int STextFormat::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "Placement", &placement );
	saver.Add( "FormatStringFileRef", &szFormatStringFileRef );

	return 0;
}

int STextFormat::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &placement );
	saver.Add( 3, &szFormatStringFileRef );

	return 0;
}



void SWindowSimpleShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSimpleShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSimpleShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );

	return 0;
}

int SWindowSimpleShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );

	return 0;
}



void SWindowSimple::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSimple", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSimple::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowSimple::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SMessageSequence::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "data", &data, pThis );
}

int SMessageSequence::operator&( IXmlSaver &saver )
{
	saver.Add( "data", &data );

	return 0;
}

int SMessageSequence::operator&( IBinSaver &saver )
{
	saver.Add( 2, &data );

	return 0;
}

DWORD SMessageSequence::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << data;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMessageSequienceEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "CustomCheckReturn", (BYTE*)&nCustomCheckReturn - pThis, sizeof(nCustomCheckReturn), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Sequience", &sequience, pThis ); 
}

int SMessageSequienceEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "CustomCheckReturn", &nCustomCheckReturn );
	saver.Add( "Sequience", &sequience );

	return 0;
}

int SMessageSequienceEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nCustomCheckReturn );
	saver.Add( 3, &sequience );

	return 0;
}

DWORD SMessageSequienceEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nCustomCheckReturn << sequience;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCheckRunScript::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CheckRunScript", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ScriptFunction", (BYTE*)&szScriptFunction - pThis, sizeof(szScriptFunction), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SCheckRunScript::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "ScriptFunction", &szScriptFunction );

	return 0;
}

int SCheckRunScript::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szScriptFunction );

	return 0;
}



void SCheckPreprogrammed::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CheckPreprogrammed", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "CheckName", (BYTE*)&szCheckName - pThis, sizeof(szCheckName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SCheckPreprogrammed::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "CheckName", &szCheckName );

	return 0;
}

int SCheckPreprogrammed::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szCheckName );

	return 0;
}



void SCheckIsWindowEnabled::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CheckIsWindowEnabled", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "WindowName", (BYTE*)&szWindowName - pThis, sizeof(szWindowName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "ParentWindowName", (BYTE*)&szParentWindowName - pThis, sizeof(szParentWindowName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SCheckIsWindowEnabled::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "WindowName", &szWindowName );
	saver.Add( "ParentWindowName", &szParentWindowName );

	return 0;
}

int SCheckIsWindowEnabled::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szWindowName );
	saver.Add( 3, &szParentWindowName );

	return 0;
}



void SCheckIsWindowVisible::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CheckIsWindowVisible", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "WindowName", (BYTE*)&szWindowName - pThis, sizeof(szWindowName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "ParentWindowName", (BYTE*)&szParentWindowName - pThis, sizeof(szParentWindowName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SCheckIsWindowVisible::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "WindowName", &szWindowName );
	saver.Add( "ParentWindowName", &szParentWindowName );

	return 0;
}

int SCheckIsWindowVisible::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szWindowName );
	saver.Add( 3, &szParentWindowName );

	return 0;
}



void SCheckIsTabActive::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CheckIsTabActive", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TabControlName", (BYTE*)&szTabControlName - pThis, sizeof(szTabControlName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Tab", (BYTE*)&nTab - pThis, sizeof(nTab), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SCheckIsTabActive::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "TabControlName", &szTabControlName );
	saver.Add( "Tab", &nTab );

	return 0;
}

int SCheckIsTabActive::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szTabControlName );
	saver.Add( 3, &nTab );

	return 0;
}



void SMessageReactionComplex::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MessageReactionComplex", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "branches", &branches, pThis );
	NMetaInfo::ReportMetaInfo( "ConditionCheck", (BYTE*)&pConditionCheck - pThis, sizeof(pConditionCheck), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "commonBefore", &commonBefore, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "commonAfter", &commonAfter, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SMessageReactionComplex::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "branches", &branches );
	saver.Add( "ConditionCheck", &pConditionCheck );
	saver.Add( "commonBefore", &commonBefore );
	saver.Add( "commonAfter", &commonAfter );

	return 0;
}

int SMessageReactionComplex::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &branches );
	saver.Add( 3, &pConditionCheck );
	saver.Add( 4, &commonBefore );
	saver.Add( 5, &commonAfter );

	return 0;
}



void SARSetGlobalVar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSetGlobalVar", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "VarName", (BYTE*)&szVarName - pThis, sizeof(szVarName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "VarValue", (BYTE*)&szVarValue - pThis, sizeof(szVarValue), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSetGlobalVar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "VarName", &szVarName );
	saver.Add( "VarValue", &szVarValue );

	return 0;
}

int SARSetGlobalVar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szVarName );
	saver.Add( 3, &szVarValue );

	return 0;
}



void SARRemoveGlobalVar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARRemoveGlobalVar", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "VarName", (BYTE*)&szVarName - pThis, sizeof(szVarName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SARRemoveGlobalVar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "VarName", &szVarName );

	return 0;
}

int SARRemoveGlobalVar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szVarName );

	return 0;
}



void SARSendUIMessage::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSendUIMessage", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "MessageID", (BYTE*)&szMessageID - pThis, sizeof(szMessageID), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "StringParam", (BYTE*)&szStringParam - pThis, sizeof(szStringParam), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "IntParam", (BYTE*)&nIntParam - pThis, sizeof(nIntParam), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSendUIMessage::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "MessageID", &szMessageID );
	saver.Add( "StringParam", &szStringParam );
	saver.Add( "IntParam", &nIntParam );

	return 0;
}

int SARSendUIMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szMessageID );
	saver.Add( 3, &szStringParam );
	saver.Add( 4, &nIntParam );

	return 0;
}



void SARSendGameMessage::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSendGameMessage", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "EventName", (BYTE*)&szEventName - pThis, sizeof(szEventName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "IntParam", (BYTE*)&nIntParam - pThis, sizeof(nIntParam), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSendGameMessage::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "EventName", &szEventName );
	saver.Add( "IntParam", &nIntParam );

	return 0;
}

int SARSendGameMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szEventName );
	saver.Add( 3, &nIntParam );

	return 0;
}



void SARSwitchTab::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSwitchTab", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TabControlName", (BYTE*)&szTabControlName - pThis, sizeof(szTabControlName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Tab", (BYTE*)&nTab - pThis, sizeof(nTab), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSwitchTab::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "TabControlName", &szTabControlName );
	saver.Add( "Tab", &nTab );

	return 0;
}

int SARSwitchTab::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &szTabControlName );
	saver.Add( 3, &nTab );

	return 0;
}



void SReactionSequenceEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Reaction", (BYTE*)&pReaction - pThis, sizeof(pReaction), NTypeDef::TYPE_TYPE_REF );
}

int SReactionSequenceEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Reaction", &pReaction );

	return 0;
}

int SReactionSequenceEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &pReaction );

	return 0;
}

DWORD SReactionSequenceEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName << pReaction;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMessageReactionsDesc::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "reactions", &reactions, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "ScriptFileRef", (BYTE*)&szScriptFileRef - pThis, sizeof(szScriptFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SMessageReactionsDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "reactions", &reactions );
	saver.Add( "ScriptFileRef", &szScriptFileRef );

	return 0;
}

int SMessageReactionsDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &reactions );
	saver.Add( 3, &szScriptFileRef );

	return 0;
}

DWORD SMessageReactionsDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << reactions;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCommandSequienceEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Sequence", &sequence, pThis ); 
}

int SCommandSequienceEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Sequence", &sequence );

	return 0;
}

int SCommandSequienceEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &sequence );

	return 0;
}

DWORD SCommandSequienceEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName << sequence;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScreenTextEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SScreenTextEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "TextFileRef", &szTextFileRef );

	return 0;
}

int SScreenTextEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &szTextFileRef );

	return 0;
}

DWORD SScreenTextEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowScreenShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScreenShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScreenShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );

	return 0;
}

int SWindowScreenShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );

	return 0;
}



void SWindowScreen::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScreen", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "MessageReactions", &messageReactions, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "CommandSequiences", &commandSequiences, pThis );
	NMetaInfo::ReportMetaInfo( "TooltipContext", (BYTE*)&nTooltipContext - pThis, sizeof(nTooltipContext), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( "RelatedTexts", &relatedTexts, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScreen::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "MessageReactions", &messageReactions );
	saver.Add( "CommandSequiences", &commandSequiences );
	saver.Add( "TooltipContext", &nTooltipContext );
	saver.Add( "RelatedTexts", &relatedTexts );

	return 0;
}

int SWindowScreen::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &messageReactions );
	saver.Add( 3, &commandSequiences );
	saver.Add( 4, &nTooltipContext );
	saver.Add( 5, &relatedTexts );

	return 0;
}



void SWindowProgressBarShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowProgressBarShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Forward", (BYTE*)&pForward - pThis, sizeof(pForward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Backward", (BYTE*)&pBackward - pThis, sizeof(pBackward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Glow", (BYTE*)&pGlow - pThis, sizeof(pGlow), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StepSize", (BYTE*)&fStepSize - pThis, sizeof(fStepSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "GlowSize", &vGlowSize, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowProgressBarShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Forward", &pForward );
	saver.Add( "Backward", &pBackward );
	saver.Add( "Glow", &pGlow );
	saver.Add( "StepSize", &fStepSize );
	saver.Add( "GlowSize", &vGlowSize );

	return 0;
}

int SWindowProgressBarShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pForward );
	saver.Add( 3, &pBackward );
	saver.Add( 4, &pGlow );
	saver.Add( 5, &fStepSize );
	saver.Add( 6, &vGlowSize );

	return 0;
}



void SWindowProgressBar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowProgressBar", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Progress", (BYTE*)&fProgress - pThis, sizeof(fProgress), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowProgressBar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "Progress", &fProgress );

	return 0;
}

int SWindowProgressBar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &fProgress );

	return 0;
}



void SProgressBarTextureInfo::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MaxValue", (BYTE*)&nMaxValue - pThis, sizeof(nMaxValue), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
}

int SProgressBarTextureInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "MaxValue", &nMaxValue );
	saver.Add( "Texture", &pTexture );

	return 0;
}

int SProgressBarTextureInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nMaxValue );
	saver.Add( 3, &pTexture );

	return 0;
}

DWORD SProgressBarTextureInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nMaxValue << pTexture;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMultiTextureProgressBarSharedState::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (BYTE*)&fValue - pThis, sizeof(fValue), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Background", (BYTE*)&pBackground - pThis, sizeof(pBackground), NTypeDef::TYPE_TYPE_REF );
}

int SMultiTextureProgressBarSharedState::operator&( IXmlSaver &saver )
{
	saver.Add( "Value", &fValue );
	saver.Add( "Background", &pBackground );

	return 0;
}

int SMultiTextureProgressBarSharedState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fValue );
	saver.Add( 3, &pBackground );

	return 0;
}

DWORD SMultiTextureProgressBarSharedState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fValue << pBackground;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowMultiTextureProgressBarShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMultiTextureProgressBarShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "States", &states, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMultiTextureProgressBarShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "States", &states );

	return 0;
}

int SWindowMultiTextureProgressBarShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &states );

	return 0;
}



void SWindowMultiTextureProgressBar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMultiTextureProgressBar", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Progresses", &progresses, pThis );
	NMetaInfo::ReportMetaInfo( "Solid", (BYTE*)&bSolid - pThis, sizeof(bSolid), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "PieceSize", (BYTE*)&fPieceSize - pThis, sizeof(fPieceSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMultiTextureProgressBar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "Progresses", &progresses );
	saver.Add( "Solid", &bSolid );
	saver.Add( "PieceSize", &fPieceSize );

	return 0;
}

int SWindowMultiTextureProgressBar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &progresses );
	saver.Add( 3, &bSolid );
	saver.Add( 4, &fPieceSize );

	return 0;
}



void SWindowTextViewShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTextViewShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Format", (BYTE*)&nFormat - pThis, sizeof(nFormat), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FontName", (BYTE*)&szFontName - pThis, sizeof(szFontName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "RedLineSpace", (BYTE*)&nRedLineSpace - pThis, sizeof(nRedLineSpace), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTextViewShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Color", &nColor );
	saver.Add( "Format", &nFormat );
	saver.Add( "FontName", &szFontName );
	saver.Add( "RedLineSpace", &nRedLineSpace );

	return 0;
}

int SWindowTextViewShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &nColor );
	saver.Add( 3, &nFormat );
	saver.Add( 4, &szFontName );
	saver.Add( 5, &nRedLineSpace );

	return 0;
}



void SWindowTextView::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTextView", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "ResizeOnTextSet", (BYTE*)&bResizeOnTextSet - pThis, sizeof(bResizeOnTextSet), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "TextFormat", (BYTE*)&pTextFormat - pThis, sizeof(pTextFormat), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTextView::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "TextFileRef", &szTextFileRef );
	saver.Add( "ResizeOnTextSet", &bResizeOnTextSet );
	saver.Add( "TextFormat", &pTextFormat );

	return 0;
}

int SWindowTextView::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &szTextFileRef );
	saver.Add( 3, &bResizeOnTextSet );
	saver.Add( 4, &pTextFormat );

	return 0;
}



void SWindowTooltipShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTooltipShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "LowerBorder", &vLowerBorder, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "HigherBorder", &vHigherBorder, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTooltipShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "LowerBorder", &vLowerBorder );
	saver.Add( "HigherBorder", &vHigherBorder );

	return 0;
}

int SWindowTooltipShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &vLowerBorder );
	saver.Add( 3, &vHigherBorder );

	return 0;
}



void SWindowTooltip::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTooltip", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTooltip::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowTooltip::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowPlayerShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowPlayerShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowPlayerShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );

	return 0;
}

int SWindowPlayerShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );

	return 0;
}



void SWindowPlayer::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowPlayer", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SequenceName", (BYTE*)&szSequenceName - pThis, sizeof(szSequenceName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "MaintainAspectRatio", (BYTE*)&bMaintainAspectRatio - pThis, sizeof(bMaintainAspectRatio), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowPlayer::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "SequenceName", &szSequenceName );
	saver.Add( "MaintainAspectRatio", &bMaintainAspectRatio );

	return 0;
}

int SWindowPlayer::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &szSequenceName );
	saver.Add( 3, &bMaintainAspectRatio );

	return 0;
}


string EnumToString( NDb::ETextEntryType eValue )
{
	switch ( eValue )
	{
	case NDb::ETET_ALL:
		return "ETET_ALL";
	case NDb::ETET_NUMERIC:
		return "ETET_NUMERIC";
	case NDb::ETET_GAME_SPY:
		return "ETET_GAME_SPY";
	case NDb::ETET_LOCAL_PLAYERNAME:
		return "ETET_LOCAL_PLAYERNAME";
	case NDb::ETET_FILENAME:
		return "ETET_FILENAME";
	default:
		return "ETET_ALL";
	}
}

NDb::ETextEntryType NDb::StringToEnum_NDb_ETextEntryType( const string &szValue )
{
	if ( szValue == "ETET_ALL" )
		return NDb::ETET_ALL;
	if ( szValue == "ETET_NUMERIC" )
		return NDb::ETET_NUMERIC;
	if ( szValue == "ETET_GAME_SPY" )
		return NDb::ETET_GAME_SPY;
	if ( szValue == "ETET_LOCAL_PLAYERNAME" )
		return NDb::ETET_LOCAL_PLAYERNAME;
	if ( szValue == "ETET_FILENAME" )
		return NDb::ETET_FILENAME;
	return NDb::ETET_ALL;
}


void SWindowEditLineShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowEditLineShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "FontName", (BYTE*)&szFontName - pThis, sizeof(szFontName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "CursorColor", (BYTE*)&nCursorColor - pThis, sizeof(nCursorColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SelColor", (BYTE*)&nSelColor - pThis, sizeof(nSelColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "LeftSpace", (BYTE*)&nLeftSpace - pThis, sizeof(nLeftSpace), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RightSpace", (BYTE*)&nRightSpace - pThis, sizeof(nRightSpace), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "YOffset", (BYTE*)&nYOffset - pThis, sizeof(nYOffset), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowEditLineShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "FontName", &szFontName );
	saver.Add( "Color", &nColor );
	saver.Add( "CursorColor", &nCursorColor );
	saver.Add( "SelColor", &nSelColor );
	saver.Add( "LeftSpace", &nLeftSpace );
	saver.Add( "RightSpace", &nRightSpace );
	saver.Add( "YOffset", &nYOffset );

	return 0;
}

int SWindowEditLineShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &szFontName );
	saver.Add( 3, &nColor );
	saver.Add( 4, &nCursorColor );
	saver.Add( 5, &nSelColor );
	saver.Add( 6, &nLeftSpace );
	saver.Add( 7, &nRightSpace );
	saver.Add( 8, &nYOffset );

	return 0;
}



void SWindowEditLine::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowEditLine", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "OnReturn", (BYTE*)&szOnReturn - pThis, sizeof(szOnReturn), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "SequienceOnReturn", &sequienceOnReturn, pThis ); 
	NMetaInfo::ReportMetaInfo( "OnEscape", (BYTE*)&szOnEscape - pThis, sizeof(szOnEscape), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "SequienceOnEscape", &sequienceOnEscape, pThis ); 
	NMetaInfo::ReportMetaInfo( "MaxLength", (BYTE*)&nMaxLength - pThis, sizeof(nMaxLength), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TextScroll", (BYTE*)&bTextScroll - pThis, sizeof(bTextScroll), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "TextEntryType", (BYTE*)&eTextEntryType - pThis, sizeof(eTextEntryType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Password", (BYTE*)&bPassword - pThis, sizeof(bPassword), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "SequienceOnTextChanged", &sequienceOnTextChanged, pThis ); 
	NMetaInfo::ReportMetaInfo( "OnTextChanged", (BYTE*)&szOnTextChanged - pThis, sizeof(szOnTextChanged), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TabOrder", (BYTE*)&nTabOrder - pThis, sizeof(nTabOrder), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "SequienceOnFocusLost", &sequienceOnFocusLost, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowEditLine::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "OnReturn", &szOnReturn );
	saver.Add( "SequienceOnReturn", &sequienceOnReturn );
	saver.Add( "OnEscape", &szOnEscape );
	saver.Add( "SequienceOnEscape", &sequienceOnEscape );
	saver.Add( "MaxLength", &nMaxLength );
	saver.Add( "TextScroll", &bTextScroll );
	saver.Add( "TextEntryType", &eTextEntryType );
	saver.Add( "Password", &bPassword );
	saver.Add( "SequienceOnTextChanged", &sequienceOnTextChanged );
	saver.Add( "OnTextChanged", &szOnTextChanged );
	saver.Add( "TabOrder", &nTabOrder );
	saver.Add( "SequienceOnFocusLost", &sequienceOnFocusLost );

	return 0;
}

int SWindowEditLine::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &szOnReturn );
	saver.Add( 3, &sequienceOnReturn );
	saver.Add( 4, &szOnEscape );
	saver.Add( 5, &sequienceOnEscape );
	saver.Add( 6, &nMaxLength );
	saver.Add( 7, &bTextScroll );
	saver.Add( 8, &eTextEntryType );
	saver.Add( 9, &bPassword );
	saver.Add( 10, &sequienceOnTextChanged );
	saver.Add( 11, &szOnTextChanged );
	saver.Add( 12, &nTabOrder );
	saver.Add( 13, &sequienceOnFocusLost );

	return 0;
}



void SWindowConsoleOutputShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowConsoleOutputShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "AutoDelete", (BYTE*)&bAutoDelete - pThis, sizeof(bAutoDelete), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowConsoleOutputShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "AutoDelete", &bAutoDelete );

	return 0;
}

int SWindowConsoleOutputShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &bAutoDelete );

	return 0;
}



void SWindowConsoleOutput::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowConsoleOutput", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowConsoleOutput::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowConsoleOutput::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowStatsSystemShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowStatsSystemShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowStatsSystemShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );

	return 0;
}

int SWindowStatsSystemShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );

	return 0;
}



void SWindowStatsSystem::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowStatsSystem", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowStatsSystem::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowStatsSystem::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowConsoleShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowConsoleShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "EditLine", (BYTE*)&pEditLine - pThis, sizeof(pEditLine), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "MakeVisible", &makeVisible, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowConsoleShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Color", &nColor );
	saver.Add( "EditLine", &pEditLine );
	saver.Add( "MakeVisible", &makeVisible );

	return 0;
}

int SWindowConsoleShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &nColor );
	saver.Add( 3, &pEditLine );
	saver.Add( 4, &makeVisible );

	return 0;
}



void SWindowConsole::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowConsole", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowConsole::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowConsole::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowScrollableContainerBaseShared::ReportMetaInfo() const
{
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Interval", (BYTE*)&nInterval - pThis, sizeof(nInterval), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "ScrollBar", (BYTE*)&pScrollBar - pThis, sizeof(pScrollBar), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Border", (BYTE*)&pBorder - pThis, sizeof(pBorder), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Selection", (BYTE*)&pSelection - pThis, sizeof(pSelection), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PreSelection", (BYTE*)&pPreSelection - pThis, sizeof(pPreSelection), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NegativeSelection", (BYTE*)&pNegativeSelection - pThis, sizeof(pNegativeSelection), NTypeDef::TYPE_TYPE_REF );
}

int SWindowScrollableContainerBaseShared::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Interval", &nInterval );
	saver.Add( "ScrollBar", &pScrollBar );
	saver.Add( "Border", &pBorder );
	saver.Add( "Selection", &pSelection );
	saver.Add( "PreSelection", &pPreSelection );
	saver.Add( "NegativeSelection", &pNegativeSelection );

	return 0;
}

int SWindowScrollableContainerBaseShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &nInterval );
	saver.Add( 3, &pScrollBar );
	saver.Add( 4, &pBorder );
	saver.Add( 5, &pSelection );
	saver.Add( 6, &pPreSelection );
	saver.Add( 7, &pNegativeSelection );

	return 0;
}

DWORD SWindowScrollableContainerBaseShared::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SWindowShared::CalcCheckSum() << nInterval << pBorder << pSelection << pPreSelection << pNegativeSelection;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowScrollableContainerBase::ReportMetaInfo() const
{
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "OnSelection", &onSelection, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "OnDoubleClick", &onDoubleClick, pThis ); 
}

int SWindowScrollableContainerBase::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "OnSelection", &onSelection );
	saver.Add( "OnDoubleClick", &onDoubleClick );

	return 0;
}

int SWindowScrollableContainerBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &onSelection );
	saver.Add( 3, &onDoubleClick );

	return 0;
}

DWORD SWindowScrollableContainerBase::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SWindow::CalcCheckSum() << onSelection << onDoubleClick;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowScrollableContainerShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScrollableContainerShared", typeID, sizeof(*this) );
	SWindowScrollableContainerBaseShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScrollableContainerShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBaseShared*)(this) );

	return 0;
}

int SWindowScrollableContainerShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBaseShared*)this );

	return 0;
}



void SWindowScrollableContainer::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScrollableContainer", typeID, sizeof(*this) );
	SWindowScrollableContainerBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScrollableContainer::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBase*)(this) );

	return 0;
}

int SWindowScrollableContainer::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBase*)this );

	return 0;
}



void SWindow1LvlTreeControlShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Window1LvlTreeControlShared", typeID, sizeof(*this) );
	SWindowScrollableContainerBaseShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ItemSample", (BYTE*)&pItemSample - pThis, sizeof(pItemSample), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SubItemSample", (BYTE*)&pSubItemSample - pThis, sizeof(pSubItemSample), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindow1LvlTreeControlShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBaseShared*)(this) );
	saver.Add( "ItemSample", &pItemSample );
	saver.Add( "SubItemSample", &pSubItemSample );

	return 0;
}

int SWindow1LvlTreeControlShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBaseShared*)this );
	saver.Add( 2, &pItemSample );
	saver.Add( 3, &pSubItemSample );

	return 0;
}



void SWindow1LvlTreeControl::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Window1LvlTreeControl", typeID, sizeof(*this) );
	SWindowScrollableContainerBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindow1LvlTreeControl::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBase*)(this) );

	return 0;
}

int SWindow1LvlTreeControl::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBase*)this );

	return 0;
}



void SWindowListHeaderShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListHeaderShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SortIconDown", (BYTE*)&pSortIconDown - pThis, sizeof(pSortIconDown), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SortIconUp", (BYTE*)&pSortIconUp - pThis, sizeof(pSortIconUp), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SubHeaderSamples", &subHeaderSamples, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListHeaderShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "SortIconDown", &pSortIconDown );
	saver.Add( "SortIconUp", &pSortIconUp );
	saver.Add( "SubHeaderSamples", &subHeaderSamples );

	return 0;
}

int SWindowListHeaderShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pSortIconDown );
	saver.Add( 3, &pSortIconUp );
	saver.Add( 4, &subHeaderSamples );

	return 0;
}



void SWindowListHeader::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListHeader", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListHeader::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowListHeader::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowListItemShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListItemShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "SubItemSamples", &subItemSamples, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListItemShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "SubItemSamples", &subItemSamples );

	return 0;
}

int SWindowListItemShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &subItemSamples );

	return 0;
}



void SWindowListItem::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListItem", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListItem::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowListItem::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowListSharedData::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListSharedData", typeID, sizeof(*this) );
	SWindowScrollableContainerBaseShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ListItem", (BYTE*)&pListItem - pThis, sizeof(pListItem), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ListHeader", (BYTE*)&pListHeader - pThis, sizeof(pListHeader), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListSharedData::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBaseShared*)(this) );
	saver.Add( "ListItem", &pListItem );
	saver.Add( "ListHeader", &pListHeader );

	return 0;
}

int SWindowListSharedData::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBaseShared*)this );
	saver.Add( 2, &pListItem );
	saver.Add( 3, &pListHeader );

	return 0;
}



void SWindowListCtrl::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowListCtrl", typeID, sizeof(*this) );
	SWindowScrollableContainerBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowListCtrl::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowScrollableContainerBase*)(this) );

	return 0;
}

int SWindowListCtrl::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowScrollableContainerBase*)this );

	return 0;
}



void SWindowTabControlShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTabControlShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "HeadersContainer", (BYTE*)&pHeadersContainer - pThis, sizeof(pHeadersContainer), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ContainerSample", (BYTE*)&pContainerSample - pThis, sizeof(pContainerSample), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ButtonSample", (BYTE*)&pButtonSample - pThis, sizeof(pButtonSample), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTabControlShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "HeadersContainer", &pHeadersContainer );
	saver.Add( "ContainerSample", &pContainerSample );
	saver.Add( "ButtonSample", &pButtonSample );

	return 0;
}

int SWindowTabControlShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pHeadersContainer );
	saver.Add( 3, &pContainerSample );
	saver.Add( 4, &pButtonSample );

	return 0;
}



void SWindowTabControl::STab::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TabContainer", (BYTE*)&pTabContainer - pThis, sizeof(pTabContainer), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ButtonName", (BYTE*)&szButtonName - pThis, sizeof(szButtonName), NTypeDef::TYPE_TYPE_STRING );
}

int SWindowTabControl::STab::operator&( IXmlSaver &saver )
{
	saver.Add( "TabContainer", &pTabContainer );
	saver.Add( "ButtonName", &szButtonName );

	return 0;
}

int SWindowTabControl::STab::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTabContainer );
	saver.Add( 3, &szButtonName );

	return 0;
}

DWORD SWindowTabControl::STab::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pTabContainer << szButtonName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowTabControl::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowTabControl", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "tabs", &tabs, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowTabControl::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "tabs", &tabs );

	return 0;
}

int SWindowTabControl::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &tabs );

	return 0;
}



void SWindowComboBoxShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowComboBoxShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Line", (BYTE*)&pLine - pThis, sizeof(pLine), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Icon", (BYTE*)&pIcon - pThis, sizeof(pIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "List", (BYTE*)&pList - pThis, sizeof(pList), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowComboBoxShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Line", &pLine );
	saver.Add( "Icon", &pIcon );
	saver.Add( "List", &pList );

	return 0;
}

int SWindowComboBoxShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pLine );
	saver.Add( 3, &pIcon );
	saver.Add( 4, &pList );

	return 0;
}



void SWindowComboBox::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowComboBox", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ListPriority", (BYTE*)&nListPriority - pThis, sizeof(nListPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MaxVisibleRows", (BYTE*)&nMaxVisibleRows - pThis, sizeof(nMaxVisibleRows), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "OnSelection", &onSelection, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowComboBox::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "ListPriority", &nListPriority );
	saver.Add( "MaxVisibleRows", &nMaxVisibleRows );
	saver.Add( "OnSelection", &onSelection );

	return 0;
}

int SWindowComboBox::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &nListPriority );
	saver.Add( 3, &nMaxVisibleRows );
	saver.Add( 4, &onSelection );

	return 0;
}


string EnumToString( NDb::EButtonSubstateType eValue )
{
	switch ( eValue )
	{
	case NDb::BST_NORMAL:
		return "BST_NORMAL";
	case NDb::BST_MOUSE_OVER:
		return "BST_MOUSE_OVER";
	case NDb::BST_PUSHED_DEEP:
		return "BST_PUSHED_DEEP";
	case NDb::BST_DISABLED:
		return "BST_DISABLED";
	case NDb::BST_RIGHT_DOWN:
		return "BST_RIGHT_DOWN";
	default:
		return "BST_NORMAL";
	}
}

NDb::EButtonSubstateType NDb::StringToEnum_NDb_EButtonSubstateType( const string &szValue )
{
	if ( szValue == "BST_NORMAL" )
		return NDb::BST_NORMAL;
	if ( szValue == "BST_MOUSE_OVER" )
		return NDb::BST_MOUSE_OVER;
	if ( szValue == "BST_PUSHED_DEEP" )
		return NDb::BST_PUSHED_DEEP;
	if ( szValue == "BST_DISABLED" )
		return NDb::BST_DISABLED;
	if ( szValue == "BST_RIGHT_DOWN" )
		return NDb::BST_RIGHT_DOWN;
	return NDb::BST_NORMAL;
}

string EnumToString( NDb::EButtonChangeStateType eValue )
{
	switch ( eValue )
	{
	case NDb::BCST_ON_PUSH:
		return "BCST_ON_PUSH";
	case NDb::BCST_ON_RELEASE:
		return "BCST_ON_RELEASE";
	default:
		return "BCST_ON_PUSH";
	}
}

NDb::EButtonChangeStateType NDb::StringToEnum_NDb_EButtonChangeStateType( const string &szValue )
{
	if ( szValue == "BCST_ON_PUSH" )
		return NDb::BCST_ON_PUSH;
	if ( szValue == "BCST_ON_RELEASE" )
		return NDb::BCST_ON_RELEASE;
	return NDb::BCST_ON_PUSH;
}


void SButtonVisualSubState::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Background", (BYTE*)&pBackground - pThis, sizeof(pBackground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Foreground", (BYTE*)&pForeground - pThis, sizeof(pForeground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "TextString", (BYTE*)&pTextString - pThis, sizeof(pTextString), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( szAddName + "OnEnterSubState", &onEnterSubState, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "TextFormat", (BYTE*)&pTextFormat - pThis, sizeof(pTextFormat), NTypeDef::TYPE_TYPE_REF );
}

int SButtonVisualSubState::operator&( IXmlSaver &saver )
{
	saver.Add( "Background", &pBackground );
	saver.Add( "Foreground", &pForeground );
	saver.Add( "TextString", &pTextString );
	saver.Add( "OnEnterSubState", &onEnterSubState );
	saver.Add( "TextFormat", &pTextFormat );

	return 0;
}

int SButtonVisualSubState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pBackground );
	saver.Add( 3, &pForeground );
	saver.Add( 4, &pTextString );
	saver.Add( 5, &onEnterSubState );
	saver.Add( 6, &pTextFormat );

	return 0;
}

DWORD SButtonVisualSubState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pBackground << pForeground << onEnterSubState;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SButtonVisualState::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Normal", &normal, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "MouseOver", &mouseOver, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pushed", &pushed, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Disabled", &disabled, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "RightButtonDown", &rightButtonDown, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "DefaultSubState", (BYTE*)&eDefaultSubState - pThis, sizeof(eDefaultSubState), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "VisualOnEnterState", &visualOnEnterState, pThis ); 
}

int SButtonVisualState::operator&( IXmlSaver &saver )
{
	saver.Add( "Normal", &normal );
	saver.Add( "MouseOver", &mouseOver );
	saver.Add( "Pushed", &pushed );
	saver.Add( "Disabled", &disabled );
	saver.Add( "RightButtonDown", &rightButtonDown );
	saver.Add( "DefaultSubState", &eDefaultSubState );
	saver.Add( "VisualOnEnterState", &visualOnEnterState );

	return 0;
}

int SButtonVisualState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &normal );
	saver.Add( 3, &mouseOver );
	saver.Add( 4, &pushed );
	saver.Add( 5, &disabled );
	saver.Add( 6, &rightButtonDown );
	saver.Add( 7, &eDefaultSubState );
	saver.Add( 8, &visualOnEnterState );

	return 0;
}

DWORD SButtonVisualState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << normal << mouseOver << pushed << disabled << rightButtonDown << eDefaultSubState << visualOnEnterState;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SButtonLogicalState::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MessageOnEnterState", (BYTE*)&szMessageOnEnterState - pThis, sizeof(szMessageOnEnterState), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "CommandsOnEnterState", &commandsOnEnterState, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "CommandsOnRightClick", &commandsOnRightClick, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "commandsOnLDblKlick", &commandsOnLDblKlick, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "WaitVisual", (BYTE*)&bWaitVisual - pThis, sizeof(bWaitVisual), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "ReverseCommands", (BYTE*)&bReverseCommands - pThis, sizeof(bReverseCommands), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
}

int SButtonLogicalState::operator&( IXmlSaver &saver )
{
	saver.Add( "MessageOnEnterState", &szMessageOnEnterState );
	saver.Add( "CommandsOnEnterState", &commandsOnEnterState );
	saver.Add( "CommandsOnRightClick", &commandsOnRightClick );
	saver.Add( "commandsOnLDblKlick", &commandsOnLDblKlick );
	saver.Add( "WaitVisual", &bWaitVisual );
	saver.Add( "ReverseCommands", &bReverseCommands );
	saver.Add( "Name", &szName );

	return 0;
}

int SButtonLogicalState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szMessageOnEnterState );
	saver.Add( 3, &commandsOnEnterState );
	saver.Add( 4, &commandsOnRightClick );
	saver.Add( 5, &commandsOnLDblKlick );
	saver.Add( 6, &bWaitVisual );
	saver.Add( 7, &bReverseCommands );
	saver.Add( 8, &szName );

	return 0;
}

DWORD SButtonLogicalState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szMessageOnEnterState << commandsOnEnterState << commandsOnRightClick << commandsOnLDblKlick << bWaitVisual << bReverseCommands << szName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowMSButtonShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMSButtonShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "VisualStates", &visualStates, pThis );
	NMetaInfo::ReportMetaInfo( "TriggerMode", (BYTE*)&eTriggerMode - pThis, sizeof(eTriggerMode), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMSButtonShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "VisualStates", &visualStates );
	saver.Add( "TriggerMode", &eTriggerMode );

	return 0;
}

int SWindowMSButtonShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &visualStates );
	saver.Add( 3, &eTriggerMode );

	return 0;
}



void SWindowMSButton::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMSButton", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "ButtonStates", &buttonStates, pThis );
	NMetaInfo::ReportMetaInfo( "ButtonGroupID", (BYTE*)&nButtonGroupID - pThis, sizeof(nButtonGroupID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AutoChangeState", (BYTE*)&bAutoChangeState - pThis, sizeof(bAutoChangeState), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "PushEffect", &pushEffect, pThis ); 
	NMetaInfo::ReportMetaInfo( "State", (BYTE*)&nState - pThis, sizeof(nState), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TextFormat", (BYTE*)&pTextFormat - pThis, sizeof(pTextFormat), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMSButton::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "ButtonStates", &buttonStates );
	saver.Add( "ButtonGroupID", &nButtonGroupID );
	saver.Add( "AutoChangeState", &bAutoChangeState );
	saver.Add( "PushEffect", &pushEffect );
	saver.Add( "State", &nState );
	saver.Add( "TextFileRef", &szTextFileRef );
	saver.Add( "TextFormat", &pTextFormat );

	return 0;
}

int SWindowMSButton::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &buttonStates );
	saver.Add( 3, &nButtonGroupID );
	saver.Add( 4, &bAutoChangeState );
	saver.Add( 5, &pushEffect );
	saver.Add( 6, &nState );
	saver.Add( 7, &szTextFileRef );
	saver.Add( 8, &pTextFormat );

	return 0;
}



void SWindowSliderShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSliderShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Horisontal", (BYTE*)&bHorisontal - pThis, sizeof(bHorisontal), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Lever", (BYTE*)&pLever - pThis, sizeof(pLever), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MinLeverSize", (BYTE*)&fMinLeverSize - pThis, sizeof(fMinLeverSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxLeverSize", (BYTE*)&fMaxLeverSize - pThis, sizeof(fMaxLeverSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSliderShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Horisontal", &bHorisontal );
	saver.Add( "Lever", &pLever );
	saver.Add( "MinLeverSize", &fMinLeverSize );
	saver.Add( "MaxLeverSize", &fMaxLeverSize );

	return 0;
}

int SWindowSliderShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &bHorisontal );
	saver.Add( 3, &pLever );
	saver.Add( 4, &fMinLeverSize );
	saver.Add( 5, &fMaxLeverSize );

	return 0;
}



void SWindowSlider::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSlider", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SpecialPositions", (BYTE*)&nSpecialPositions - pThis, sizeof(nSpecialPositions), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSlider::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "SpecialPositions", &nSpecialPositions );

	return 0;
}

int SWindowSlider::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &nSpecialPositions );

	return 0;
}



void SWindowScrollBarShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScrollBarShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ButtonLower", (BYTE*)&pButtonLower - pThis, sizeof(pButtonLower), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ButtonGreater", (BYTE*)&pButtonGreater - pThis, sizeof(pButtonGreater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Slider", (BYTE*)&pSlider - pThis, sizeof(pSlider), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScrollBarShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "ButtonLower", &pButtonLower );
	saver.Add( "ButtonGreater", &pButtonGreater );
	saver.Add( "Slider", &pSlider );

	return 0;
}

int SWindowScrollBarShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &fSpeed );
	saver.Add( 3, &pButtonLower );
	saver.Add( 4, &pButtonGreater );
	saver.Add( 5, &pSlider );

	return 0;
}



void SWindowScrollBar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowScrollBar", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Effects", &effects, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowScrollBar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "Effects", &effects );

	return 0;
}

int SWindowScrollBar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &effects );

	return 0;
}



void SUISButtonSubstate::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISButtonSubstate", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Substate", (BYTE*)&eSubstate - pThis, sizeof(eSubstate), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "WaitTime", (BYTE*)&fWaitTime - pThis, sizeof(fWaitTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "szButton", &szButton, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISButtonSubstate::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "Substate", &eSubstate );
	saver.Add( "WaitTime", &fWaitTime );
	saver.Add( "szButton", &szButton );

	return 0;
}

int SUISButtonSubstate::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &eSubstate );
	saver.Add( 3, &fWaitTime );
	saver.Add( 4, &szButton );

	return 0;
}

}
using namespace NDb;
BASIC_REGISTER_DATABASE_CLASS( SUIDesc )
BASIC_REGISTER_DATABASE_CLASS( SUIStateBaseShared )
BASIC_REGISTER_DATABASE_CLASS( SUIStateBase )
REGISTER_DATABASE_CLASS( 0x11075C00, SUISMoveTo ) 
REGISTER_DATABASE_CLASS( 0x11075C05, SUISRunReaction ) 
REGISTER_DATABASE_CLASS( 0x11075C07, SUISSendUIMessage ) 
REGISTER_DATABASE_CLASS( 0x110953C1, SUIConsoleCommand ) 
REGISTER_DATABASE_CLASS( 0x210C5480, SUISDirectRunReaction ) 
BASIC_REGISTER_DATABASE_CLASS( SBackground )
REGISTER_DATABASE_CLASS( 0x1106CB03, SBackgroundSimpleScallingTexture ) 
REGISTER_DATABASE_CLASS( 0x1106F441, SBackgroundSimpleTexture ) 
REGISTER_DATABASE_CLASS( 0x1106CB02, SBackgroundTiledTexture ) 
BASIC_REGISTER_DATABASE_CLASS( SWindowBaseShared )
BASIC_REGISTER_DATABASE_CLASS( SWindowBaseDesc )
BASIC_REGISTER_DATABASE_CLASS( SWindowShared )
BASIC_REGISTER_DATABASE_CLASS( SWindow )
REGISTER_DATABASE_CLASS( 0x1106CB40, SForegroundTextStringShared ) 
REGISTER_DATABASE_CLASS( 0x1106CB42, SForegroundTextString ) 
REGISTER_DATABASE_CLASS( 0x17159C40, STextFormat ) 
REGISTER_DATABASE_CLASS( 0x11082C40, SWindowSimpleShared ) 
REGISTER_DATABASE_CLASS( 0x1107C380, SWindowSimple ) 
REGISTER_DATABASE_CLASS( 0x1106BC41, SCheckRunScript ) 
REGISTER_DATABASE_CLASS( 0x1106BC42, SCheckPreprogrammed ) 
REGISTER_DATABASE_CLASS( 0x15083380, SCheckIsWindowEnabled ) 
REGISTER_DATABASE_CLASS( 0x110B3400, SCheckIsWindowVisible ) 
REGISTER_DATABASE_CLASS( 0x170B6300, SCheckIsTabActive ) 
REGISTER_DATABASE_CLASS( 0x1106BC43, SMessageReactionComplex ) 
REGISTER_DATABASE_CLASS( 0x1106BC44, SARSetGlobalVar ) 
REGISTER_DATABASE_CLASS( 0x1106BC45, SARRemoveGlobalVar ) 
REGISTER_DATABASE_CLASS( 0x1106BC46, SARSendUIMessage ) 
REGISTER_DATABASE_CLASS( 0x15084340, SARSendGameMessage ) 
REGISTER_DATABASE_CLASS( 0x15083384, SARSwitchTab ) 
REGISTER_DATABASE_CLASS( 0x1106BC48, SWindowScreenShared ) 
REGISTER_DATABASE_CLASS( 0x1106BC4A, SWindowScreen ) 
REGISTER_DATABASE_CLASS( 0x1106C405, SWindowProgressBarShared ) 
REGISTER_DATABASE_CLASS( 0x1106C402, SWindowProgressBar ) 
REGISTER_DATABASE_CLASS( 0x150A0AC1, SWindowMultiTextureProgressBarShared ) 
REGISTER_DATABASE_CLASS( 0x150A0AC2, SWindowMultiTextureProgressBar ) 
REGISTER_DATABASE_CLASS( 0x1106C3C2, SWindowTextViewShared ) 
REGISTER_DATABASE_CLASS( 0x1106C3C4, SWindowTextView ) 
REGISTER_DATABASE_CLASS( 0x1106C409, SWindowTooltipShared ) 
REGISTER_DATABASE_CLASS( 0x1106C40A, SWindowTooltip ) 
REGISTER_DATABASE_CLASS( 0x170A7B80, SWindowPlayerShared ) 
REGISTER_DATABASE_CLASS( 0x170A7B81, SWindowPlayer ) 
REGISTER_DATABASE_CLASS( 0x1106C340, SWindowEditLineShared ) 
REGISTER_DATABASE_CLASS( 0x1106C342, SWindowEditLine ) 
REGISTER_DATABASE_CLASS( 0x11095B02, SWindowConsoleOutputShared ) 
REGISTER_DATABASE_CLASS( 0x11095B01, SWindowConsoleOutput ) 
REGISTER_DATABASE_CLASS( 0x110AC480, SWindowStatsSystemShared ) 
REGISTER_DATABASE_CLASS( 0x110AC4C0, SWindowStatsSystem ) 
REGISTER_DATABASE_CLASS( 0x1106C303, SWindowConsoleShared ) 
REGISTER_DATABASE_CLASS( 0x1106C304, SWindowConsole ) 
BASIC_REGISTER_DATABASE_CLASS( SWindowScrollableContainerBaseShared )
BASIC_REGISTER_DATABASE_CLASS( SWindowScrollableContainerBase )
REGISTER_DATABASE_CLASS( 0x170AF300, SWindowScrollableContainerShared ) 
REGISTER_DATABASE_CLASS( 0x1107C381, SWindowScrollableContainer ) 
REGISTER_DATABASE_CLASS( 0x1106C3C0, SWindow1LvlTreeControlShared ) 
REGISTER_DATABASE_CLASS( 0x1106C3C1, SWindow1LvlTreeControl ) 
REGISTER_DATABASE_CLASS( 0x1106C2C0, SWindowListHeaderShared ) 
REGISTER_DATABASE_CLASS( 0x1106C2C2, SWindowListHeader ) 
REGISTER_DATABASE_CLASS( 0x170CC480, SWindowListItemShared ) 
REGISTER_DATABASE_CLASS( 0x1106C2C3, SWindowListItem ) 
REGISTER_DATABASE_CLASS( 0x1106C301, SWindowListSharedData ) 
REGISTER_DATABASE_CLASS( 0x1106C302, SWindowListCtrl ) 
REGISTER_DATABASE_CLASS( 0x1106C440, SWindowTabControlShared ) 
REGISTER_DATABASE_CLASS( 0x1106C442, SWindowTabControl ) 
REGISTER_DATABASE_CLASS( 0x17122380, SWindowComboBoxShared ) 
REGISTER_DATABASE_CLASS( 0x17122381, SWindowComboBox ) 
REGISTER_DATABASE_CLASS( 0x1106C380, SWindowMSButtonShared ) 
REGISTER_DATABASE_CLASS( 0x1106C382, SWindowMSButton ) 
REGISTER_DATABASE_CLASS( 0x1106C401, SWindowSliderShared ) 
REGISTER_DATABASE_CLASS( 0x1106C400, SWindowSlider ) 
REGISTER_DATABASE_CLASS( 0x1106C383, SWindowScrollBarShared ) 
REGISTER_DATABASE_CLASS( 0x1106C384, SWindowScrollBar ) 
REGISTER_DATABASE_CLASS( 0x170AE340, SUISButtonSubstate ) 

