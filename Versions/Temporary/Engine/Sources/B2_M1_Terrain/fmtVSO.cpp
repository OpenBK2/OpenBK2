#include "StdAfx.h"

#include "fmtVSO.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"

int SVectorStripeObjectPoint::operator&( IXmlSaver &saver )
{
	
	saver.Add( "Pos", &vPos );
	saver.Add( "Norm", &vNorm );
	saver.Add( "Radius", &fRadius );
	saver.Add( "Width", &fWidth );
	saver.Add( "KeyPoint", &bKeyPoint );
	saver.Add( "Opacity", &fOpacity );
	return 0;
}


int SVectorStripeObjectDesc::SLayer::operator&( IXmlSaver &saver )
{
	
	saver.Add( "OpacityCenter", &opacityCenter );
	saver.Add( "OpacityBorder", &opacityBorder );
	saver.Add( "StreamSpeed", &fStreamSpeed );
	saver.Add( "TextureStep", &fTextureStep );
	saver.Add( "NumCells", &nNumCells );
	saver.Add( "Animated", &bAnimated );
	saver.Add( "Texture", &szTexture );
	saver.Add( "Disturbance", &fDisturbance );
	saver.Add( "RelWidth", &fRelWidth );
	return 0;
}


int SVectorStripeObjectDesc::SLayer::operator&( IBinSaver &saver )
{
	
	saver.Add( 1, &opacityCenter );
	saver.Add( 2, &opacityBorder );
	saver.Add( 3, &fStreamSpeed );
	saver.Add( 4, &fTextureStep );
	saver.Add( 5, &nNumCells );
	saver.Add( 6, &bAnimated );
	saver.Add( 7, &szTexture );
	saver.Add( 8, &fDisturbance );
	saver.Add( 9, &fRelWidth );
	return 0;
}


int SVectorStripeObjectDesc::operator&( IXmlSaver &saver )
{
	
	
	saver.Add( "Type", &eType );
	saver.Add( "Priority", &nPriority );
	saver.Add( "Passability", &fPassability );
	saver.Add( "AIClasses", &dwAIClasses );
	saver.Add( "Bottom", &bottom );
	saver.Add( "BottomBorders", &bottomBorders );
	saver.Add( "Layers", &layers );
	saver.Add( "MiniMapCenterColor", &miniMapCenterColor );
	saver.Add( "MiniMapBorderColor", &miniMapBorderColor );
	saver.Add( "AmbientSound", &szAmbientSound );
	saver.Add( "SoilParams", &cSoilParams );

	return 0;
}


int SVectorStripeObjectDesc::operator&( IBinSaver &saver )
{
	

	saver.Add( 1, &bottom );
	saver.Add( 2, &bottomBorders );
	saver.Add( 3, &layers );
	saver.Add( 4, &miniMapCenterColor );
	saver.Add( 5, &miniMapBorderColor );
	saver.Add( 6, &nPriority );
	saver.Add( 7, &eType );
	saver.Add( 8, &fPassability );
	saver.Add( 9, &szAmbientSound );
	saver.Add( 10, &dwAIClasses );
	saver.Add( 11, &cSoilParams );

	return 0;
}


int SVectorStripeObject::operator&( IXmlSaver &saver )
{
	
	saver.AddTypedSuper( static_cast<SVectorStripeObjectDesc*>(this) );
	saver.Add( "DescName", &szDescName );
	saver.Add( "Points", &points );
	saver.Add( "ControlPoints", &controlpoints );
	saver.Add( "ID", &nID );
	// read descriptor in the case of existance
	if ( saver.IsReading() && !szDescName.empty() )
	{
		CFileStream stream( NVFS::GetMainVFS(), szDescName + ".xml" );
		if ( stream.IsOk() ) 
		{
			CPtr<IXmlSaver> saver1 = CreateXmlSaver( &stream, SAVER_MODE_READ );
			SVectorStripeObjectDesc desc;
			saver1->Add( "VSODescription", &desc );
			*( static_cast<SVectorStripeObjectDesc*>(this) ) = desc;
		}
	}
	return 0;
}


int SVectorStripeObject::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<SVectorStripeObjectDesc*>(this) );
	saver.Add( 2, &points );
	saver.Add( 3, &controlpoints );
	saver.Add( 4, &nID );
	saver.Add( 5, &szDescName );
	// read descriptor in the case of existance
	if ( saver.IsReading() && !szDescName.empty() )
	{
		CFileStream stream( NVFS::GetMainVFS(), szDescName + ".xml" );
		if ( stream.IsOk() )
		{
			CPtr<IXmlSaver> saver1 = CreateXmlSaver( &stream, SAVER_MODE_READ );
			SVectorStripeObjectDesc desc;
			saver1->Add( "VSODescription", &desc );
			*( static_cast<SVectorStripeObjectDesc*>(this) ) = desc;
		}
	}
	return 0;
}

