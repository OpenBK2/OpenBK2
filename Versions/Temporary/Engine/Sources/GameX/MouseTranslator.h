#pragma once

#include "input/gamemessage.h"

class CScene;
class CMapObj;
class CSelector;
struct IGameEvent;

enum EButtonState
{
	BS_NONE,
	BS_LEFT_DOWN,
	BS_RIGHT_DOWN
};
enum EActionState
{
	AS_NONE,
	AS_SCROLL,
	AS_SELECT,
	AS_TARGET,
	AS_MOVE
};

#define SA_PRESERVE               0x00000000   //
#define SA_CLEAR_IF_NEW_NOT_EMPTY	0x00000001   //  нельзя комбинировать
#define SA_CLEAR_ALWAYS           0x00000002   //
#define SA_INVERSE								0x00000003   //
#define SA_PREVIOS_SELECTION			0x0000000F

#define SA_ONE_TYPE								0x00000010
#define SA_ON_WORLD								0x00000020
#define SA_SELECT_BY_RECT					0x00000040

/*****************************************************************************************************************************
События от MouseTranslator:        Параметры события:
Map:
  scroll_map				                 вектор (AI coords)
Selection:
  start_selection                    вектор
	update_selection                   вектор
	end_selection                      SA_???????
	cancel_selection
Target:
  set_target												 вектор
	set_target_minimap								 вектор
Movement:
  set_destrination									 вектор
	set_destrination_minimap					 вектор
	set_direction                      вектор  - возникает, если во время указания точки движения нажать Alt
	cancel_direction
Default Action:
  do_action                          вектор

p.s.: посколькe для всех событий вектор - это то что прислали (т.е. координаты относительно экрана) поэтому
  к чему относиться конкретно вектор должно определять само приложение. Исключение состовляют 
	???????????_minimap - в этом случае координаты в единицах AI


bTwoPointAction указывает на необходимоть генерации события для двух точек, с возможностью
отмены действия, если между нажатием на кнопку мыши и ее отпусканием был нажат ESC. При
это генерируются события из секции Target
*****************************************************************************************************************************/

class CMouseTranslator : protected NInput::CGMORegContainer, public CObjectBase
{
	CPtr<CSelector> pSelector;

	EButtonState	eButtonState;
	EActionState	eActionState;

	NInput::CBind bindShift;
	NInput::CBind bindCtrl;
	NInput::CBind bindAlt;

	bool bDownTargetMode;

	CVec2	vDownPos;
	bool bDownSamePos;

	NTimer::STime downTime;
	bool bDownSameTime;
//	bool bCanDoRotate;
private:
	void MsgCancelAction( const SGameMessage &msg );

	void ResetState()
	{
		bDownTargetMode = false;
		bDownSamePos = false;

		eButtonState = BS_NONE;
		eActionState = AS_NONE;
	}

	int GetFlags() const;
protected:
	bool IsSelected( class CMapObj *pMO ) const;

	bool IsDownTargetMode() const { return bDownTargetMode; }

	bool IsDownSamePos() const { return bDownSamePos; }
	bool IsDownSamePos( const CVec2 &vPos ) const;
	bool IsDownSameTime() const { return bDownSameTime; }
	bool IsDownSameTime( const NTimer::STime &curTime ) const;
//	bool IsCanDoRotate() const { return bCanDoRotate; }

	void RaiseEvent( const char *pszEventName ) 
	{
		NInput::PostEvent( pszEventName, 0, 0 );
	}
	void RaiseEvent( const char *pszEventName, int nParam ) 
	{
		NInput::PostEvent( pszEventName, nParam, 0 );
	}
	void RaiseEvent( const char *pszEventName, const CVec2 &vPos ) 
	{
		NInput::PostEvent( pszEventName, PackCoords( vPos ), 0 );
	}
	void RaiseEventAndFlags( const char *pszEventName, int nParam ) 
	{
		NInput::PostEvent( pszEventName, nParam, GetFlags() );
	}
	void RaiseEventAndFlags( const char *pszEventName, const CVec2 &vPos ) 
	{
		NInput::PostEvent( pszEventName, PackCoords( vPos ), GetFlags() );
	}

	EActionState GetActionState() const { return eActionState; }
	void SetActionState( const EActionState value ) { eActionState = value; }

	virtual void OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect ) = 0;
	virtual void OnButtonUp( const CVec2 &vPos ) = 0;
	virtual void OnLButtonDown( const CVec2 &vPos, bool bObject ) = 0;
	virtual void OnRButtonDown( const CVec2 &vPos, bool bObject ) = 0;
	virtual void OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode ) = 0;
	virtual void OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode ) = 0;

	virtual void CancelAction();
public:
	CMouseTranslator( CSelector *_pSelector );

	void DoLButtonDblClk( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode );
	void DoMouseMove( const CVec2 &vPos, bool bMouseMoveDirect )
	{ 
		if ( eActionState != AS_NONE )
			OnMouseMove( vPos, bMouseMoveDirect );
	}
	void DoLButtonDown( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode );
	void DoLButtonUp( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode );
	void DoRButtonDown( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode );
	void DoRButtonUp( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode );

	void DoMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode )
	{
		eButtonState = BS_LEFT_DOWN;
		OnMinimapLButtonDown( vPos, bTargetMode );
	}
	void DoMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode )
	{
		eButtonState = BS_RIGHT_DOWN;
		OnMinimapRButtonDown( vPos, bTargetMode );
	}

	virtual bool ProcessEvent( const SGameMessage &event );

	virtual void OnGetFocus( bool bFocus );

	int operator&( IBinSaver &saver );

	bool IsShiftDown() const { return bindShift.IsActive(); }
	bool IsCtrlDown() const { return bindCtrl.IsActive(); }
	bool IsAltDown() const { return bindAlt.IsActive(); }
};

class CMouseTranslatorB1 : public CMouseTranslator
{
	OBJECT_NOCOPY_METHODS( CMouseTranslatorB1 );
	bool bUpdateDir;
protected:
	void OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect );
	void OnButtonUp( const CVec2 &vPos );
	void OnLButtonDown( const CVec2 &vPos, bool bObject );
	void OnRButtonDown( const CVec2 &vPos, bool bObject );
	void OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode );
	void OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode );
public:
	CMouseTranslatorB1() : CMouseTranslator( 0 )	{ };
	CMouseTranslatorB1( CSelector *_pSelector ) : CMouseTranslator( _pSelector )	{ };
};

class CMouseTranslatorB2Base : public CMouseTranslator
{
	ZDATA_(CMouseTranslator)
	ZONSERIALIZE
	bool bWasForcedAction;
public:	
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMouseTranslator*)this); OnSerialize( f ); f.Add(2,&bWasForcedAction); return 0; }
private:
	void InitPrivate();
	void OnSerialize( IBinSaver &f )
	{
		if ( f.IsReading() )
			ResetState();
	}
	void ResetState();
	void CheckForcedAction();
protected:
	void OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect );
	void OnButtonUp( const CVec2 &vPos );
	void OnLButtonDown( const CVec2 &vPos, bool bObject );
	void OnRButtonDown( const CVec2 &vPos, bool bObject );
	void OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode );
	void OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode );

	void CancelAction();
	
	void MsgNotifyForcedAction( const SGameMessage &msg );

	bool ProcessEvent( const SGameMessage &event );
public:
	CMouseTranslatorB2Base();
	CMouseTranslatorB2Base( CSelector *_pSelector );
};

class CMouseTranslatorB2Game : public CMouseTranslatorB2Base
{
	OBJECT_NOCOPY_METHODS( CMouseTranslatorB2Game );
	ZDATA_( CMouseTranslatorB2Base )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMouseTranslatorB2Base *)this); return 0; }
public:
	CMouseTranslatorB2Game() {}
	CMouseTranslatorB2Game( CSelector *_pSelector )
		: CMouseTranslatorB2Base( _pSelector )
	{
	}
};

class CMouseTranslatorB2Replay : public CMouseTranslatorB2Base
{
	OBJECT_NOCOPY_METHODS( CMouseTranslatorB2Replay );

	ZDATA_( CMouseTranslatorB2Base )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMouseTranslatorB2Base *)this); return 0; }
protected:
	void OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect )
	{
		CMouseTranslatorB2Base::OnMouseMove( vPos, bMouseMoveDirect );
	}
	void OnButtonUp( const CVec2 &vPos ) {}
	void OnLButtonDown( const CVec2 &vPos, bool bObject ) {}
	void OnRButtonDown( const CVec2 &vPos, bool bObject ) {}
	void OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode )
	{
		CMouseTranslatorB2Base::OnMinimapLButtonDown( vPos, bTargetMode );
	}
	void OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode )
	{
		CMouseTranslatorB2Base::OnMinimapRButtonDown( vPos, bTargetMode );
	}
public:
	CMouseTranslatorB2Replay() {}
	CMouseTranslatorB2Replay( CSelector *_pSelector )
		: CMouseTranslatorB2Base( _pSelector )
	{
	}
};


