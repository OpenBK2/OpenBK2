#include "StdAfx.h"
#include "RenderNode.h"
#include "GScene.h"
#include "ObjectFader.h"

namespace NGScene
{

CObjectFader::CObjectFader( CObjectBase *_pObj, CFuncBase<float> *_pFader, vector<CPtr<CFuncBase<float> > > *_pTranspChannels ) : pObj(_pObj), pFader(_pFader)
{
	if ( _pTranspChannels )
	{
		transpChannels.reserve( _pTranspChannels->size() );
		for ( int k = 0; k < _pTranspChannels->size(); ++k )
			transpChannels.push_back( (*_pTranspChannels)[k].GetPtr() );
	}
}

bool CObjectFader::Update( void *p )
{
	if ( pFader && IsValid( pObj ) )
	{
		pFader.Refresh();
		const float fFadeVal = pFader->GetValue();

		if ( CDynamicCast<CRenderNode> pNode = pObj )
		{
			for ( int k = 0; k < pNode->parts.size(); ++k )
			{
				if ( k < transpChannels.size() && IsValid(transpChannels[k]) )
				{
					transpChannels[k].Refresh();
					SetPartFade( pNode->parts[k], fFadeVal * transpChannels[k]->GetValue() );
				}
				else
					SetPartFade( pNode->parts[k], fFadeVal );
			}
			return true;
		}
	}
  return false;
}

}

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x13168B80, CObjectFader )
