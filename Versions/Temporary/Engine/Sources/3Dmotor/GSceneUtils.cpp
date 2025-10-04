#include "stdafx.h"
#include "GSceneUtils.h"
#include "3DLib/Transform.h"

namespace NGScene
{

// CMSRNode

void CMSRNode::Recalc()
{
	Multiply( &value.forward, pAncestor->GetValue().forward, pPos->GetValue().forward );
	Multiply( &value.backward, pPos->GetValue().backward, pAncestor->GetValue().backward );
	//SHMatrix mTest = value.forward * value.backward;
}

int CMSRNode::operator&( CStructureSaver &f )
{
	f.Add( 1, &pAncestor ); 
	f.Add( 2, &pPos );
	return 0;
}

// CMNode

void CMNode::Recalc()
{
	const SFBTransform &src = pAncestor->GetValue();
	MultiplyTranslate( &value.forward, src.forward, pMove->GetValue() );
	MultiplyInvTranslate( &value.backward, src.backward, pMove->GetValue() );
}

// CScaleNode

void CScaleNode::Recalc()
{
	const CTRect<int> &src = pSize->GetValue();

	value.x1 = src.x1 * vScreenRect.x / 1024;
	value.y1 = src.y1 * vScreenRect.y / 768;
	value.x2 = value.x1 + src.Width();
	value.y2 = value.y1 + src.Height();
}

int CScaleNode::operator&( CStructureSaver &f )
{
	f.Add( 1, &vScreenRect ); 
	f.Add( 2, &pSize ); 
	return 0;
}

// CMSRConvert

void CMSRConvert::Recalc()
{
	SHMatrix s;
	CMatrixStack43<4> m;
	const CVec3 &rotate = pRotate->GetValue();
	const CVec3 &scale = pScale->GetValue();
	MakeMatrix( &s, rotate.x, rotate.y, rotate.z, pMove->GetValue() );
	m.Init( s );
	m.PushScale( scale.x, scale.y, scale.z );
	value.forward = m.Get();
	value.backward.HomogeneousInverse( value.forward );
	// CRAP}
}

int CMSRConvert::operator&( CStructureSaver &f )
{
	f.Add( 1, &pMove ); 
	f.Add( 2, &pRotate );
	f.Add( 3, &pScale );
	return 0;
}

// CVec3Convert

void CVec3Convert::Recalc()
{
	value.x = pX->GetValue();
	value.y = pY->GetValue();
	value.z = pZ->GetValue();
}

int CVec3Convert::operator&( CStructureSaver &f )
{
	f.Add( 1, &pX ); 
	f.Add( 2, &pY );
	f.Add( 3, &pZ );
	return 0;
}

// CSinus

void CSinus::Recalc()
{
	value = fpAdd + fpMn * sin( float( pTime->GetValue() / 1000 ) * fpFreq * FP_2PI);
}

int CSinus::operator&( CStructureSaver &f )
{
	f.Add( 1, &fpMn );
	f.Add( 2, &fpFreq );
	f.Add( 3, &fpAdd );
	f.Add( 4, &pTime );
	return 0;
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02511005, CMSRNode )
REGISTER_SAVELOAD_CLASS( 0x00821180, CMNode )
REGISTER_SAVELOAD_CLASS( 0x0251100d, CCMSR )
REGISTER_SAVELOAD_CLASS( 0xB0118000, CCVec2 )
REGISTER_SAVELOAD_CLASS( 0x0251100e, CCVec3 )
REGISTER_SAVELOAD_CLASS( 0x021a2150, CCVec4 )
REGISTER_SAVELOAD_CLASS( 0x03031620, CCInt )
REGISTER_SAVELOAD_CLASS( 0x0251100f, CCFloat )
REGISTER_SAVELOAD_CLASS( 0x02831000, CCTRect )
REGISTER_SAVELOAD_CLASS( 0x02831001, CCWString )
REGISTER_SAVELOAD_CLASS( 0x02511010, CCFBTransform )
REGISTER_SAVELOAD_CLASS( 0x02511011, CMSRConvert )
REGISTER_SAVELOAD_CLASS( 0x02831151, CCRectLayout )
REGISTER_SAVELOAD_CLASS( 0x02511012, CVec3Convert )
REGISTER_SAVELOAD_CLASS( 0x02511013, CSinus )
REGISTER_SAVELOAD_CLASS( 0x12341181, CScaleNode )
REGISTER_SAVELOAD_CLASS( 0x114c1110, CCTPoint )
REGISTER_SAVELOAD_CLASS( 0x022a2120, CExtractTranslation )

extern "C" void __cdecl ForceGSceneGraph() {}

