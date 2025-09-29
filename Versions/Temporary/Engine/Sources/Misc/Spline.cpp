#include "stdafx.h"

#include "Spline.h"
#pragma optimize ( "p", on )
#pragma optimize ( "g", on )
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAnalyticBSpline2::N_PARTS_FOR_CLOSEST_POINT_SEARCHING = 10;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAnalyticBSpline2::GetClosestPoint( const CVec2 &vPoint, CVec2 *pvClosestPoint, float *pfT, const float fT0, const float fT1 )
{
	*pfT = -1.0f;
	*pvClosestPoint = VNULL2;
	float fBestDist2 = 0.0f;

	const float fAdd = ( fT1 - fT0 ) / N_PARTS_FOR_CLOSEST_POINT_SEARCHING;
	if ( fabs(fAdd) >= 1.0f / N_PARTS_FOR_CLOSEST_POINT_SEARCHING )
	{
		float fCurT = fT0;
		for ( int i = 0; i <= N_PARTS_FOR_CLOSEST_POINT_SEARCHING; ++i )
		{
			CVec2 vCandidate = Get( fCurT );
			const float fCandidateDist2 = fabs2( vCandidate - vPoint );

			if ( *pfT == -1.0f || fCandidateDist2 < fBestDist2 )
			{
				*pfT = fCurT;
				fBestDist2 = fCandidateDist2;
				*pvClosestPoint = vCandidate;
			}

			fCurT += fAdd;
		}
	}
	else
	{
		*pfT = fT0;
		*pvClosestPoint = Get( fT0 );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ************************************************************************************************************************ //
// **
// ** beta spline surface
// ** Источник: "Начала компьютерной графики" под редакцией Шикина Е. В.
// **
// **
// ************************************************************************************************************************ //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// вычисление постоянных коэффицентов
// fBeta1, fBeta2 задают форму кривой
void CBetaSpline3D::Init( float _fBeta1, float _fBeta2 )
{
	fBeta1 = _fBeta1;
	fBeta2 = _fBeta2;
	invdelta = 1.0f / ( 2.0f * fBeta1 * fBeta1 * fBeta1
		+ 4.0f * fBeta1 * fBeta1
		+ 4.0f * fBeta1 + fBeta2 + 2.0f );

	VolumeCoeffs( _fBeta1, _fBeta2 );
	fBeta1_3 = 2 * fBeta1 * fBeta1 * fBeta1 * invdelta;
	fBeta1_2 = 2 * fBeta1 * fBeta1 * invdelta;

	fBeta1 *= 2 * invdelta;
	fBeta2 *= invdelta;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// весовые функциональные коэффициенты
inline float CBetaSpline3D::b_2( const float t[3] ) const
{
	return fBeta1_3 * square( 1 - t[0] ) * ( 1 - t[0] );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::b_1( const float t[3] ) const
{
	return fBeta1_3 * t[0] * ( t[1] - 3*t[0] + 3 ) + 
		fBeta1_2 * ( t[2] - 3*t[1] + 2 ) + 
		fBeta1 * ( t[2] - 3*t[0] + 2 ) + 
		fBeta2 * ( 2 * t[2] - 3 * t[1] + 1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::b0( const float t[3] ) const
{
	return fBeta1_2 * t[1] * ( 3 - t[0] ) + fBeta1 * t[0] * ( 3 - t[1] ) + fBeta2 * t[1] * ( 3 - 2*t[0] ) + invdelta * 2 * ( 1 - t[2] );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::b1( const float t[3] ) const
{
	return 2 * t[2] * invdelta;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// производные функциональных коэффициентов
inline float CBetaSpline3D::db_2( const float t[3] ) const
{
	return -3.0f * fBeta1_3 * square( 1 - t[0] );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::db_1( const float t[3] ) const
{
	return 3.0f * (fBeta1_3 * square( t[0] - 1 ) + fBeta1_2 * (t[0] - 2) * t[0] 
	+ fBeta1 * (t[1] - 1) + fBeta2 * 2 * (t[0] - 1) * t[0]);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::db0( const float t[3] ) const
{
	return 3.0f * (fBeta1 + (fBeta1_2 * 2 + fBeta2 * 2) * t[0] - (invdelta * 2 + fBeta1 + fBeta1_2 + fBeta2 * 2) * t[1] );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::db1( const float t[3] ) const
{
	return 6 * t[1] * invdelta;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Вычисление интерполированного значения для параметров 0<= u,v <= 1
// ptControls - массив контрольных точек
// в строке вершины вдоль параметра u; считается, что точки расположены в след. порядке: 
// v(-2, l), v(-1, l), v(0, l), (1, l)
// в столбце вдоль параметра v; в следующем порядке v(j, -2), v(j, -1), v(j, -0), (j, 1)
// Полученное интерполированное значение принадлежит интервалу v1( -1, -1 ) - v2( 0, 0 )
// ( v1 соответствует u=v=0,  v2  u=v=1 - "элементарная поверхность" ) 
const float CBetaSpline3D::Value( float u, float v, const float ptCtrls[16] ) const
{
	float t[3];
	t[0] = u;
	t[1] = u * u;
	t[2] = t[1] * u;
	const float bj[4] = { b_2( t ), b_1( t ), b0( t ), b1( t ) };

	t[0] = v;
	t[1] = v * v;
	t[2] = t[1] * v;
	float bl[4] = { b_2( t ), b_1( t ), b0( t ), b1( t ) };

	// внутреннее суммирование по параметру v
	return	
		bj[0] * ( bl[0] * ptCtrls[0] + bl[1] * ptCtrls[4] + bl[2] * ptCtrls[8]	+ bl[3] * ptCtrls[12] ) +
		bj[1] * ( bl[0] * ptCtrls[1] + bl[1] * ptCtrls[5] + bl[2] * ptCtrls[9]  + bl[3] * ptCtrls[13] ) +
		bj[2] * ( bl[0] * ptCtrls[2] + bl[1] * ptCtrls[6] + bl[2] * ptCtrls[10] + bl[3] * ptCtrls[14] ) +
		bj[3] * ( bl[0] * ptCtrls[3] + bl[1] * ptCtrls[7] + bl[2] * ptCtrls[11] + bl[3] * ptCtrls[15] );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBetaSpline3D::GetNormale( CVec3 *pvNormale, float u, float v, const float ptCtrls[16] ) const
{
	float t[3];
	t[0] = u;
	t[1] = u * u;
	t[2] = t[1] * u;
	const float bj[4]  = { b_2( t ), b_1( t ), b0( t ), b1( t ) };
	const float dbj[4] = { db_2( t ), db_1( t ), db0( t ), db1( t ) };

	t[0] = v;
	t[1] = v * v;
	t[2] = t[1] * v;
	const float bl[4] = { b_2( t ), b_1( t ), b0( t ), b1( t ) };
	const float dbl[4] = { db_2( t ), db_1( t ), db0( t ), db1( t ) };

	const float z1 = 
		dbj[0] * ( bl[0] * ptCtrls[0] + bl[1] * ptCtrls[4] + bl[2] * ptCtrls[8]  + bl[3] * ptCtrls[12] ) +
		dbj[1] * ( bl[0] * ptCtrls[1] + bl[1] * ptCtrls[5] + bl[2] * ptCtrls[9]  + bl[3] * ptCtrls[13] ) +
		dbj[2] * ( bl[0] * ptCtrls[2] + bl[1] * ptCtrls[6] + bl[2] * ptCtrls[10] + bl[3] * ptCtrls[14] ) +
		dbj[3] * ( bl[0] * ptCtrls[3] + bl[1] * ptCtrls[7] + bl[2] * ptCtrls[11] + bl[3] * ptCtrls[15] );
	const float x1 = (  bl[0] +  bl[1] +  bl[2] +  bl[3] ) * ( -2 * dbj[0] - dbj[1] + dbj[3] );
	const float y1 = ( dbj[0] + dbj[1] + dbj[2] + dbj[3] ) * ( -2 *  bl[0] -  bl[1] +  bl[3] );

	const float z2 = 
		bj[0] * ( dbl[0] * ptCtrls[0] + dbl[1] * ptCtrls[4] + dbl[2] * ptCtrls[8]  + dbl[3] * ptCtrls[12] ) +
		bj[1] * ( dbl[0] * ptCtrls[1] + dbl[1] * ptCtrls[5] + dbl[2] * ptCtrls[9]  + dbl[3] * ptCtrls[13] ) +
		bj[2] * ( dbl[0] * ptCtrls[2] + dbl[1] * ptCtrls[6] + dbl[2] * ptCtrls[10] + dbl[3] * ptCtrls[14] ) +
		bj[3] * ( dbl[0] * ptCtrls[3] + dbl[1] * ptCtrls[7] + dbl[2] * ptCtrls[11] + dbl[3] * ptCtrls[15] );
	const float x2 = ( dbl[0] + dbl[1] + dbl[2] + dbl[3] ) * ( -2 *  bj[0] -  bj[1] +  bj[3] );
	const float y2 = (  bj[0] +  bj[1] +  bj[2] +  bj[3] ) * ( -2 * dbl[0] - dbl[1] + dbl[3] );

	pvNormale->x = y1 * z2 - y2 * z1;
	pvNormale->y = x2 * z1 - z2 * x1;
	pvNormale->z = x1 * y2 - y1 * x2;

	Normalize( pvNormale );
	/*
	// внутреннее суммирование по параметру v
	CVec3 ptDU, ptDV;
	ptDU  = dbj[0] * ( bl[0] * ptCtrls1[0] + bl[1] * ptCtrls1[4] + bl[2] * ptCtrls1[8] + bl[3] * ptCtrls1[12] );
	ptDU += dbj[1] * ( bl[0] * ptCtrls1[1] + bl[1] * ptCtrls1[5] + bl[2] * ptCtrls1[9]  + bl[3] * ptCtrls1[13] );
	ptDU += dbj[2] * ( bl[0] * ptCtrls1[2] + bl[1] * ptCtrls1[6] + bl[2] * ptCtrls1[10] + bl[3] * ptCtrls1[14] );
	ptDU += dbj[3] * ( bl[0] * ptCtrls1[3] + bl[1] * ptCtrls1[7] + bl[2] * ptCtrls1[11] + bl[3] * ptCtrls1[15] );

	ptDV  = bj[0] * ( dbl[0] * ptCtrls1[0] + dbl[1] * ptCtrls1[4] + dbl[2] * ptCtrls1[8] + dbl[3] * ptCtrls1[12] );
	ptDV += bj[1] * ( dbl[0] * ptCtrls1[1] + dbl[1] * ptCtrls1[5] + dbl[2] * ptCtrls1[9]  + dbl[3] * ptCtrls1[13] );
	ptDV += bj[2] * ( dbl[0] * ptCtrls1[2] + dbl[1] * ptCtrls1[6] + dbl[2] * ptCtrls1[10] + dbl[3] * ptCtrls1[14] );
	ptDV += bj[3] * ( dbl[0] * ptCtrls1[3] + dbl[1] * ptCtrls1[7] + dbl[2] * ptCtrls1[11] + dbl[3] * ptCtrls1[15] );

	*pvNormale = -( ptDU ^ ptDV );
	*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBetaSpline3D::VolumeCoeffs( float b1, float b2 )
{
	float d = square( invdelta ) / 4.0f;
	float b12 = b1  * b1;
	float b13 = b12 * b1;
	float b14 = b13 * b1;
	float b15 = b14 * b1;
	float b16 = b15 * b1;

	fVolCoeffs[0] = d * b16;
	fVolCoeffs[1] = d * ( 3*b16 + 5*b15 + 3*b14 + b2*b13 );
	fVolCoeffs[2] = d * ( 3*b15 + 5*b14 + b2*b13 + 3*b13 );
	fVolCoeffs[3] = d * b13;

	fVolCoeffs[4 + 0] = fVolCoeffs[1];
	fVolCoeffs[4 + 1] = d * ( 9*b16 + 30*b15 + 43*b14 + 6*b2*b13 + 30*b13 + 10*b2*b12 + 9*b12 + 6*b2*b1 + b2*b2 );
	fVolCoeffs[4 + 2] = d * ( 9*b15 + 30*b14 + 3*b2*b13 + 43*b13 + 8*b2*b12 + 30*b12 + 8*b2*b1 + 9*b1 + b2*b2 + 3*b2 );
	fVolCoeffs[4 + 3] = d * ( 3*b13 + 5*b12 + 3*b1 + b2 );

	fVolCoeffs[8 + 0] = fVolCoeffs[2];
	fVolCoeffs[8 + 1] = fVolCoeffs[4 + 2];
	fVolCoeffs[8 + 2] = d * ( 9*b14 + 30*b13 + 6*b2*b12 + 43*b12 + 10*b2*b1 + 30*b1 + b2*b2 + 6*b2 + 9 );
	fVolCoeffs[8 + 3] = d * ( 3*b12 + 5*b1 + b2 + 3 );

	fVolCoeffs[12 + 0] = fVolCoeffs[3];
	fVolCoeffs[12 + 1] = fVolCoeffs[4 + 3];
	fVolCoeffs[12 + 2] = fVolCoeffs[8 + 3];
	fVolCoeffs[12 + 3] = d;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Вспомогательные функции для вычисления средней высоты
// t[0] - степени коэффициента t0  ( первая-4тая степень )
// t[1] - степени коэффициента t1  ( первая-4тая степень )
inline float CBetaSpline3D::F00( const float t[2][4] ) const
{
	return -4*t[0][0] + 6*t[0][1] - 4*t[0][2] + t[0][3] - t[1][0]*(-4 + 6*t[1][0] - 4*t[1][1] + t[1][2]);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F11( const float t[2][4] ) const
{
	return 9*t[0][0] - 5*t[0][2] + 2*t[0][3] - 9*t[1][0] + 5*t[1][2] - 2*t[1][3];
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F22( const float t[2][4] ) const
{
	return 2*t[0][3] - 3*t[0][2] - 3*t[0][1] - 2*t[0][0] + t[1][0]*(-2*t[1][2] + 3*t[1][1] + 3*t[1][0] + 2);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F33( const float t[2][4] ) const
{
	return t[0][3] - t[1][3];
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F01( const float s[2][4], const float t[2][4] ) const
{
	return (9*s[0][0] - 5*s[0][2] + 2*s[0][3] - 9*s[1][0] + 5*s[1][2] - 2*s[1][3]) * 
		(-4*t[0][0] + 6*t[0][1] - 4*t[0][2] + t[0][3] - t[1][0] * (-4 + 6*t[1][0] - 4*t[1][1] + t[1][2]) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F02( const float s[2][4], const float t[2][4] ) const
{
	return (-2*s[0][0] - 3*s[0][1] - 3*s[0][2] + 2*s[0][3] + s[1][0] * (2 + 3*s[1][0] + 3*s[1][1] - 2*s[1][2])) * 
		(-4*t[0][0] + 6*t[0][1] - 4*t[0][2] + t[0][3] - t[1][0] * (-4 + 6*t[1][0] - 4*t[1][1] + t[1][2]));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F03( const float s[2][4], const float t[2][4] ) const
{
	return (s[0][3] - s[1][3]) * 
		(-4*t[0][0] + 6*t[0][1] - 4*t[0][2] + t[0][3] - t[1][0]*(-4 + 6*t[1][0] - 4*t[1][1] + t[1][2]));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F12( const float s[2][4], const float t[2][4] ) const
{
	return (-2*s[0][0] - 3*s[0][1] - 3*s[0][2] + 2*s[0][3] + s[1][0] * (2 + 3*s[1][0] + 3*s[1][1] - 2*s[1][2])) 
		* (9*t[0][0] - 5*t[0][2] + 2*t[0][3] - 9*t[1][0] + 5*t[1][2] - 2*t[1][3]);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F13( const float s[2][4], const float t[2][4] ) const
{
	return (s[0][3] - s[1][3]) * (9*t[0][0] - 5*t[0][2] + 2*t[0][3] - 9*t[1][0] + 5*t[1][2] - 2*t[1][3]);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float CBetaSpline3D::F23( const float s[2][4], const float t[2][4] ) const
{
	return (s[0][3] - s[1][3]) * 
		(-2*t[0][0] - 3*t[0][1] - 3*t[0][2] + 2*t[0][3] + t[1][0] * (2 + 3*t[1][0] + 3*t[1][1] - 2*t[1][2]));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CVec3 GetNURBSPoint( const float t, const CVec3 &p1, const CVec3 &p2, const CVec3 &p3, const CVec3 &p4,
                            const float fW1, const float fW2, const float fW3, const float fW4 )
{
  const float n1 = ( 1.0f - t ) * ( 1.0f - t ) * ( 1.0f - t ) * 0.16666666f;
  const float n2 = ( 3.0f * t * t * t - 6.0f * t * t + 4.0f ) * 0.16666666f;
  const float n3 = ( -3.0f * t * t * t + 3.0f * t * t + 3.0f * t + 1.0f ) * 0.16666666f;
  const float n4 = t * t * t * 0.16666666f;
  NI_ASSERT( fabs2( fW1 * n1 + fW2 * n2 + fW3 * n3 + fW4 * n4 ) > EPS_VALUE, "NURBS determinant is 0" );
  const float w = 1.0f / ( fW1 * n1 + fW2 * n2 + fW3 * n3 + fW4 * n4 );
  return CVec3( ( fW1 * n1 * p1.x + fW2 * n2 * p2.x + fW3 * n3 * p3.x + fW4 * n4 * p4.x ) * w,
                ( fW1 * n1 * p1.y + fW2 * n2 * p2.y + fW3 * n3 * p3.y + fW4 * n4 * p4.y ) * w,
                ( fW1 * n1 * p1.z + fW2 * n2 * p2.z + fW3 * n3 * p3.z + fW4 * n4 * p4.z ) * w );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SampleNURBSCurve( vector<CVec3> *pRes, const float fSampleStep, const vector<CVec3> &ctrlPoints,
  const float fWeight1/* = 1.0f*/, const float fWeight2/* = 1.0f*/, const float fWeight3/* = 1.0f*/, const float fWeight4/* = 1.0f*/ )
{
  pRes->reserve( 512 );
  float fStep, fPrevDist, fDist;
  int nStepsNum;
  for ( int i = 0; i < ctrlPoints.size(); ++i )
  {
    const int nInd1 = i;
    const int nInd2 = ( i + 1 ) % ctrlPoints.size();
    const int nInd3 = ( i + 2 ) % ctrlPoints.size();
    const int nInd4 = ( i + 3 ) % ctrlPoints.size();
    //
    fPrevDist = fabs2( GetNURBSPoint( 1.0f, ctrlPoints[nInd1], ctrlPoints[nInd2], ctrlPoints[nInd3], ctrlPoints[nInd4],
                                      fWeight1, fWeight2, fWeight3, fWeight4 ) -
                       GetNURBSPoint( 0.0f, ctrlPoints[nInd1], ctrlPoints[nInd2], ctrlPoints[nInd3], ctrlPoints[nInd4],
                                      fWeight1, fWeight2, fWeight3, fWeight4 ) );
    fStep = 0.5f;
    nStepsNum = 1;
    while ( ( ( fDist = ( fabs2( GetNURBSPoint( fStep, ctrlPoints[nInd1], ctrlPoints[nInd2], ctrlPoints[nInd3], ctrlPoints[nInd4],
                                   fWeight1, fWeight2, fWeight3, fWeight4 ) -
                                 GetNURBSPoint( 0.0f, ctrlPoints[nInd1], ctrlPoints[nInd2], ctrlPoints[nInd3], ctrlPoints[nInd4],
                                   fWeight1, fWeight2, fWeight3, fWeight4 ) ) ) ) > fSampleStep ) ||
            ( fPrevDist > fSampleStep ) )
    {
      fStep *= 0.5f;
      fPrevDist = fDist;
      nStepsNum *= 2;
    }
    //fStep *= 2.0f;
    nStepsNum <<= 1;
    //
    for ( int g = 0; g < nStepsNum; ++g )
    {
      pRes->push_back( GetNURBSPoint( (float)g * fStep, ctrlPoints[nInd1], ctrlPoints[nInd2], ctrlPoints[nInd3], ctrlPoints[nInd4],
                                      fWeight1, fWeight2, fWeight3, fWeight4 ) );
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma optimize ( "", off )
#pragma optimize ( "", on )
