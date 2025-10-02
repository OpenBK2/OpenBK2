#pragma once
namespace NDb
{
	struct STexture;
}

namespace NML
{

IVisReflowObject* CreateTextObject( CMLStream *pStream, int nStart, int nSize );
IVisReflowObject* CreateImageObject( NDb::STexture *pTexture, const CTPoint<int> &size, int nBorder );
IVisReflowObject* CreateTabObject();
IVisReflowObject* CreateSpringObject( int nSize );

} // NAMESPACE


