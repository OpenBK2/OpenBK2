#include "stdafx.h"
#include "Tools.h"


namespace NUITools
{

void ApplyWindowAlign( NDb::EPositionAllign eAlign, float fParentPos, float fParentSize,
								float fChildPos, 
								float fLowerMargin, float fHigherMargin,
								float *nSize,	/* in, out */
								float *pScreenPos /* out */ )
{
	switch( eAlign )
	{
	case 	NDb::EPA_LOW_END:
		*pScreenPos = fParentPos + fChildPos;
		break;
	case 	NDb::ERA_CENTER:
		*pScreenPos = fParentPos + Float2Int( (fParentSize - int(*nSize) ) / 2 ) + fChildPos;
		break;
	case 	NDb::EPA_HIGH_END:
		*pScreenPos = fParentPos + fParentSize - *nSize + fChildPos;
		break;
	case NDb::EPA_MARGIN:
		*pScreenPos = fParentPos + fLowerMargin;
		*nSize = fParentSize - fLowerMargin - fHigherMargin;
		break;
	default:
		*pScreenPos = fParentPos + fChildPos;
		break;
	}
}

void ApplyTextureAllign( const NDb::EPositionAllign eAllign,
												 const float fWidth, const float fTextureWidth,
												 float *pfMap1, float *pfMap2,
												 float *pfPos )
{
	switch( eAllign )
	{
	case NDb::EPA_LOW_END:
		*pfMap1 = 0;
		*pfMap2 = (std::min)( fWidth, fTextureWidth );

		break;
	case NDb::ERA_CENTER:
		//CRAP{ NOT IMPLEMENTED, WONDER EVER NEEDED
		NI_ASSERT( false, "not allowed ERA_CENTER" );
		//CRAP}
		break;
	case NDb::EPA_HIGH_END:
		if ( fWidth < fTextureWidth )
		{
			*pfMap1 = fTextureWidth - fWidth;
			*pfMap2 = fTextureWidth;
		}
		else
		{
			*pfPos += fWidth - fTextureWidth;
			*pfMap1 = 0.0f;
			*pfMap2 = fTextureWidth;
		}
		break;
	case NDb::EPA_MARGIN:
		//CRAP{ NOT IMPLEMENTED, MAY BE NEEDED TO REMOVE BackgroundSimpleScallingTexture
		NI_ASSERT( false, "not allowed ERA_CENTER" );
		//CRAP}

		break;
	default:
		*pfMap1 = 0;
		*pfMap2 = (std::min)( fWidth, fTextureWidth );
		break;
	}
}

void ApplyPlacement( const NDb::SWindowPlacement &placement, const CTRect<float> &parentRect, 
	CTRect<float> *pRect )
{
	float nSizeX = pRect->GetSizeX();
	float nSizeY = pRect->GetSizeY();
	
	if ( placement.horAllign.IsValid() )
	{
		NUITools::ApplyWindowAlign( placement.horAllign.Get(), parentRect.x1, parentRect.GetSizeX(),
			placement.position.Get().x,
			placement.lowerMargin.Get().x, placement.upperMargin.Get().x,
			&nSizeX, &pRect->x1 );
	}

	if ( placement.verAllign.IsValid() )
	{
		NUITools::ApplyWindowAlign( placement.verAllign.Get(), parentRect.y1, parentRect.GetSizeY(),
			placement.position.Get().y,
			placement.lowerMargin.Get().y, placement.upperMargin.Get().y,
			&nSizeY, &pRect->y1 );
	}

	pRect->x2 = pRect->x1 + nSizeX;
	pRect->y2 = pRect->y1 + nSizeY;
}

}

