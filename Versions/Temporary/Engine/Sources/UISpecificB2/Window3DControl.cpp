#include "StdAfx.h"
#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "ui/ui.h"
#include "input/gamemessage.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "3dmotor/rectlayout.h"
#include "Window3DControl.h"
#include "SceneB2/Scene.h"
#include "UI/UIVisitor.h"
#include "3DMotor/GAnimation.hpp"
#include "Main/GameTimer.h"

#include <zconf.h>

// CWindow3DControl

CWindow3DControl::CWindow3DControl() :
	bIsObjectsVisible( false )
{
}

CWindow3DControl::~CWindow3DControl()
{
	HideObjects();
}

void CWindow3DControl::Visit( struct IUIVisitor *pVisitor )
{
	pVisitor->SetUpperLevel();
	CWindow::Visit( pVisitor );
	pVisitor->SetLowerLevel();

	CTRect<float> rcClear = GetWindowRect();

	CTPoint<float> ptPosition( 0, 0 );
	CTRect<float> rcClip( 0, 0, 1024, 768 );
	CRectLayout rcLayout;
	rcLayout.AddRect( rcClear.left, rcClear.top, rcClear.Width(), rcClear.Height(), CTRect<float>(0,0,256,256) );
	VirtualToScreen( &rcLayout );
	VirtualToScreen( &rcClip );
	VirtualToScreen( &ptPosition );
	pVisitor->VisitZClearRect( rcLayout, ptPosition, rcClip );
	
	UpdateObjects( false );
}

void CWindow3DControl::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	pInstance = checked_cast<const NDb::SWindow3DControl*>( pDesc )->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindow3DControlShared*>( pInstance->pShared );
	rectPrev.SetEmpty();
	nBaseID3D = 0;

	CWindow::InitByDesc( pDesc );
}

void CWindow3DControl::SetObjects( const vector<SObject> &_objects )
{
	if ( objects != _objects )
	{
		HideObjects();
		objects = _objects;
		UpdateObjects( true );
	}
	else
		UpdateObjects( false );
}

void CWindow3DControl::HideObjects()
{
	if ( bIsObjectsVisible )
	{
		for ( int i = 0; i < objects.size(); ++i )
		{
			struct SObject &obj = objects[i];
			Scene()->RemoveInterfaceObject( GetScreen(), nBaseID3D + i );
		}
		bIsObjectsVisible = false;
	}
}

void CWindow3DControl::UpdateObjects( bool bForced )
{
	bool bShow = IsVisible();
	CTRect<float> rect = GetWindowRect();
	if ( bIsObjectsVisible == bShow && rectPrev == rect && !bForced )
		return;

	HideObjects();

	if ( bShow )
	{
		for ( int i = 0; i < objects.size(); ++i )
		{
			struct SObject &obj = objects[i];
//			NI_ASSERT( obj.pModel, "No model" );
			if ( !obj.pModel )
				continue;
			Scene()->AddInterfaceObject( GetScreen(), nBaseID3D + i, obj.pModel, CVec2( obj.vPos.x + rect.x1, obj.vPos.y + rect.y1 ), obj.vSize );
			
			NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetInterfaceObjAnimator( GetScreen(), nBaseID3D + i );
			if ( pAnimator && obj.pAnim )
			{
				const NTimer::STime startTime = Singleton<IGameTimer>()->GetAbsTime();
				pAnimator->AddAnimation( startTime, NAnimation::SAnimHandle(obj.pAnim, 0), true );
			}
		}
		rectPrev = rect;
		bIsObjectsVisible = true;
	}
}

void CWindow3DControl::OnChangeVisibility( bool bShow )
{
	UpdateObjects( false );
}

IWindow3DControl::SParam CWindow3DControl::GetDBObjectParam( int nIndex ) const
{
	SParam param;
	if ( pShared && nIndex < pShared->places.size() )
	{
		const NDb::SWindow3DControlShared::SObjectParams &dbParam = pShared->places[nIndex];
		param.vPos = dbParam.vPos;
		param.vSize = dbParam.vSize;
	}
	else
	{
		param.vPos = VNULL2;
		param.vSize = VNULL2;
	}
		
	return param;
}

void CWindow3DControl::SetBaseID3D( int nID )
{
	nBaseID3D = nID;
}

REGISTER_SAVELOAD_CLASS(0x17176440, CWindow3DControl)


