#pragma once

namespace NML
{

const int N_FONTOBJECT_NAME = 0x00000001;
const int N_FONTOBJECT_SIZE = 0x00000002;
const int N_FONTOBJECT_OUTLINESIZE = 0x00000004;
const int N_FONTOBJECT_OUTLINECOLOR = 0x00000008;
const int N_FONTOBJECT_FORCEFONTSIZE = 0x00000010;

IReflowObject* CreateLineBreakObject();
IReflowObject* CreateColorObject( const NGfx::SPixel8888 &color );
IReflowObject* CreateHAlignObject( EHAlign nAlign );
IReflowObject* CreateVAlignObject( EVAlign nAlign );
IReflowObject* CreateFontObject( int nFlags, const NGScene::SFont &font, const NGfx::SPixel8888 &outlineColor, int nOutlineSize, bool bForceFontSize );
IReflowObject* CreateMinFontSizeObject( int nSize );

} // NAMESPACE

