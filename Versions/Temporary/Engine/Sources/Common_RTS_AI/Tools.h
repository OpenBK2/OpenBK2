#pragma once

#include "Common_RTS_AI/AIMap.h"
#include "System/FastMath.h"

const int SPLINE_STEP = 6;
const int SPLINE_N_OF_ITERATONS = 50;

class CBasePathUnit;

class CBSpline
{
public: int operator&( IBinSaver &saver ); private:;

				CVec2 a, b, c, d;
				CVec2 x, dx, d2x, d3x;
				CVec2 fw_dx, fw_d2x, fw_d3x;
				float t, tForward;
				BYTE cntToForward;
public:
	CBSpline() : a( VNULL2 ), b( VNULL2 ), c( VNULL2 ), d( VNULL2 ), 
		x( VNULL2 ), dx( VNULL2 ), d2x( VNULL2 ), d3x( VNULL2 ), 
		fw_dx( VNULL2 ), fw_d2x( VNULL2 ), fw_d3x( VNULL2 ), t( 0 ), tForward( 0 ),
		cntToForward( 0 ) {}

		// для вычисления сплайна
		const static float DELTA;
		// для просмотра вперёд на предмет залоканных тайлов на пути
		const static float DELTA_FORWARD;
		const static int N_OF_ITERATONS;
		const static int N_ITERS_TO_FORWARD;

		void Init( const CVec2 &p3, const CVec2 &p2, const CVec2 &p1, const CVec2 &p0 );

		void Iterate();

		const CVec2& GetPoint() const { return x; }
		const CVec2& GetDX() const { return dx; }
		const float GetReverseR() const;

		struct SForwardIter
		{
			float t;
			CVec2 x;
			CVec2 fw_dx, fw_d2x, fw_d3x;
		};

		// если pIter->t == -1, то дальше итерировать нельзя, т.к. сплайн кончился
		const void StartForwardIterating( SForwardIter *pIter );
		const void IterateForward( SForwardIter *pIter );

		void DumpState() const;
};

// путь по окружности
class CCirclePath
{
	float fStartDir, fDirDiff;
	CVec2 vCenter;
	float fRadius;
	bool bClockWise;

	float fCurrentDelta;    // текущее приращение
	CVec2 vX, vDX;				  // координата точки и направление касательной в этой точке
	CVec2 vLastX, vLastDX;  // координата последние точки и направление в этой точке

	bool bForwardDir;				// едем вперед или нет

public:
	CCirclePath()
		: fStartDir( 0 ), fDirDiff( 0 ), vCenter( 0, 0 ), fRadius( 0.0f ), bClockWise( false ),
		fCurrentDelta( 0 ), vX( 0, 0 ), vDX( 0, 0 ), vLastX( 0, 0 ), vLastDX( 0, 0 ), bForwardDir( true ) {} 

		CCirclePath( const WORD _wStartDir, const WORD _wDirDiff, const CVec2 &_vPoint, const float _fRadius, const bool _bClockWise, const bool _bForward )
			: fRadius( _fRadius ), bClockWise( _bClockWise ), fCurrentDelta( 0 ), bForwardDir( _bForward )
		{
			const WORD wPerpDir = ( bClockWise == bForwardDir ) ? ( _wStartDir + 16386 ) : ( _wStartDir - 16386 );
			vCenter = _vPoint - fRadius * GetVectorByDirection( wPerpDir );
			vDX = GetVectorByDirection( _wStartDir );

			const CVec2 vEndDX = GetVectorByDirection( ( bClockWise ) ? ( _wStartDir - _wDirDiff ) : ( _wStartDir + _wDirDiff ) );
			const float fEndDir = ( vEndDX.x == 0 ) ? ( ( vEndDX.y > 0 ) ? FP_PI2 : -FP_PI2 ) : atan2f( vEndDX.y, vEndDX.x );
			fStartDir =  ( vDX.x == 0 ) ? ( ( vDX.y > 0 ) ? FP_PI2 : -FP_PI2 ) : atan2f( vDX.y, vDX.x );
			fDirDiff = ( bClockWise ) ? ( fStartDir - fEndDir ) : ( fEndDir - fStartDir );
			while ( fDirDiff < 0 )
				fDirDiff += FP_2PI;

			vX = _vPoint;
			vLastDX = CVec2( NMath::Cos( fEndDir ), NMath::Sin( fEndDir ) );
			if ( !bForwardDir )
			{
				vLastDX = -vLastDX;
				vDX = -vDX;
			}
			const float fEndPerp =  ( bClockWise == bForwardDir ) ? ( fEndDir + FP_PI2 ) : ( fEndDir - FP_PI2 );
			vLastX = CVec2( fRadius*NMath::Cos( fEndPerp ), fRadius*NMath::Sin( fEndPerp ) ) + vCenter;
		}

		const CVec2 &GetX() const { return vX; }
		const CVec2 &GetDX() const { return vDX; }

		const CVec2 &GetLastX() const { return vLastX; }
		const CVec2 &GetLastDX() const { return vLastDX; }

		const bool IsForwardDir() const { return bForwardDir; }
		// fLength - длина пути, которую необходимо пройти. Возвращаемое значение, сколько еще можно пройти,
		// т.е. не нулевое, если завершили идти по этой окружности
		float Iterate( const float fLength );

		int operator&( IBinSaver &saver );
};

//! вызвать func для тайлов на окружности начиная от wStartAngle до wStartAngle+wDiffAngle, и до тех
//! пор, пока func возвращает true. Возвращает дельту между начальным углом и последним углом, где 
//! func вернул true
template<class TYPE>
WORD CheckArcTiles( TYPE &func, const CVec2 &vUnit, const WORD wStartAngle, const WORD wDiffAngle, const float fRadius, const bool bClockWise, const bool bForward, CAIMap *pAIMap )
{
	const WORD wPerpAngle = ( bClockWise == bForward ) ? ( wStartAngle - 16384 ) : ( wStartAngle + 16384 );
	const CVec2 vCenter = vUnit + fRadius * GetVectorByDirection( wPerpAngle );
	SVector tile = SVector( -666, -666 );
	const float fDelta = atan2f( 1, fRadius ) * 65536.0f / FP_2PI;
	const WORD wDelta = fDelta;
	for ( WORD angle = 0; angle < wDiffAngle; angle += wDelta )
	{
		const WORD wAngle = ( bClockWise ) ? ( wPerpAngle - angle ) : ( wPerpAngle + angle );
		const SVector curTile = pAIMap->GetTile( vCenter - fRadius * GetVectorByDirection( wAngle ) );
		if ( tile != curTile )
		{
			tile = curTile;
			if ( !func( tile ) )
				return angle;
		}
	}
	return wDiffAngle;
}

const SRect GetUnitFullSpeedRect( const CBasePathUnit *pUnit, const bool bForInfantry );
const SRect GetUnitSpeedRect( const CBasePathUnit *pUnit, const bool bForInfantry );
const SRect GetUnitSmallRect( const CBasePathUnit *pUnit );
const SRect GetUnitNormalRect( const CBasePathUnit *pUnit );


