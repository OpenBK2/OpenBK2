#include "stdafx.h"

namespace NImage
{

/*template<class T>
void MakeE( CArray2D<T> *pMatrix )
{
	pMatrix->FillZero();
	for ( int x = 0; x < Min( pMatrix->GetSizeX(), pMatrix->GetSizeY() ); ++x )
		(*pMatrix)[x][x] = 1;
}

template<class T>
	bool InvertRobust( CArray2D<T> *pMatrix )
{
	const T about0( 1.0e-20 );
	const T zero(0);
	ASSERT( pMatrix->GetSizeX() == pMatrix->GetSizeY() );
	if ( pMatrix->GetSizeX() != pMatrix->GetSizeY() )
		return false;
	int nSize = pMatrix->GetSizeX();
	CArray2D<T> right, left( *pMatrix );
	right.SetSizes( nSize, nSize );
	MakeE( &right );
	for ( int i=0; i<nSize; i++ )
	{
		T diag=left[i][i], diag1=left[i][i];
		int maxi = i;
		for ( int k=i+1; k<nSize; k++ ) 
		{
			if ( fabs(left[k][i]) > diag1 )
			{
				diag1 = left[k][i]; 
				maxi = k;
			}
		}
		if ( maxi != i && fabs(diag)*((T)10) < fabs(diag1) )
		{
			for ( int u=0; u<nSize; u++ )
			{
				left[i][u] += left[maxi][u];
				right[i][u] += right[maxi][u];
			}
			diag = left[i][i];
		}
		if ( fabs(diag) < about0 )
		{
			int h = i;
			while ( ( h < nSize-1 ) && ( fabs(diag) < about0 ) )
			{
				h++;
				if ( fabs(left[h][i]) > about0 )
				{
					for ( int u=0; u<nSize; u++ )
					{
						left[i][u] += left[h][u];
						right[i][u] += right[h][u];
					}
					diag = left[i][i];
				}
			}
			if ( fabs(diag) < about0 )
				return false;
		}
		T invdiag;
		invdiag=((T)1)/diag;
		for ( int j=0; j<nSize; j++ )
		{
			left[i][j] *= invdiag;
			right[i][j] *= invdiag;
		}
		for ( int k=i+1; k<nSize; k++ )
		{
			T  koef = left[k][i];
			T *le = &left[k][0], *lei = &left[i][0];
			T *ri = &right[k][0], *rii = &right[i][0];
			T *lefin = le + nSize;
			if ( koef != zero )
			{
				//	for( s=0; s<size; s++){ left[k][s]-=koef*left[i][s]; right[k][s]-=koef*right[i][s]; }
				while ( le < lefin )
				{
					le[0] -= koef*lei[0]; le++; lei++; 
					ri[0] -= koef*rii[0]; ri++; rii++;
				}
			}
		}
	}
	for ( int i=nSize-1; i>=0; i-- ) 
	{
		for ( int k=0; k<i; k++ ) 
		{
			T  koef = left[k][i];
			T *le = &left[k][0], *lei = &left[i][0];
			T *ri = &right[k][0], *rii = &right[i][0];
			T *lefin = le+nSize;
			if ( koef != zero )
			{
				//	for( s=0; s<size; s++){ left[k][s]-=koef*left[i][s]; right[k][s]-=koef*right[i][s];}
				while ( le < lefin )
				{
					le[0] -= koef*lei[0]; le++; lei++; 
					ri[0] -= koef*rii[0]; ri++; rii++;
				}
			}
		}
	}
	*pMatrix = right;
	return true;
}*/

struct SSquare
{
	float a[2][2];
};
static float CalcIntegral( const SSquare &a, const SSquare & b )
{
	float fRes = ( 1 / 36.0f ) * ( 
		4 * ( 
		a.a[0][0] * b.a[0][0] + 
		a.a[0][1] * b.a[0][1] + 
		a.a[1][0] * b.a[1][0] + 
		a.a[1][1] * b.a[1][1] 
		) +
		2 * ( 
		a.a[0][0] * b.a[0][1] + a.a[0][0] * b.a[1][0] +
		a.a[0][1] * b.a[0][0] + a.a[0][1] * b.a[1][1] +
		a.a[1][0] * b.a[1][1] + a.a[1][0] * b.a[0][0] +
		a.a[1][1] * b.a[1][0] + a.a[1][1] * b.a[0][1]
		) + 
		(
		a.a[0][0] * b.a[1][1] + 
		a.a[0][1] * b.a[1][0] + 
		a.a[1][0] * b.a[0][1] + 
		a.a[1][1] * b.a[0][0] 
		)
		);
	return fRes;
}

static float Multiply( const CArray2D<float> &a, const CArray2D<float> &b )
{
	ASSERT( a.GetSizeX() == b.GetSizeX() && a.GetSizeY() == b.GetSizeY() );
	float fSum = 0;
	for ( int x = 0; x < a.GetSizeX() - 1; ++x )
	{
		for ( int y = 0; y < a.GetSizeY() - 1; ++y )
		{
			SSquare sa, sb;
			for ( int dx = 0; dx < 2; ++dx )
			{
				for ( int dy = 0; dy < 2; ++dy )
				{
					sa.a[dy][dx] = a[y+dy][x+dx];
					sb.a[dy][dx] = b[y+dy][x+dx];
				}
			}
			fSum += CalcIntegral( sa, sb );
		}
	}
	return fSum;
}

/*static void NormalizeArr( CArray2D<float> *_p )
{
	CArray2D<float> &p = *_p;
	float fSum = Multiply( p, p );;
	fSum = 1 / sqrt( fSum );
	for ( int x = 0; x < p.GetSizeX(); ++x )
	{
		for ( int y = 0; y < p.GetSizeY(); ++y )
		{
			p[y][x] *= fSum;
		}
	}
}*/

/*
static void ShiftMatrix( const CArray2D<float> &src, CArray2D<float> *p, int nDX, int nDY )
{
	CArray2D<float> &res = *p;
	res = src;
	res.FillZero();
	int nMinX = Max( 0, -nDX ), nMaxX = Min( res.GetSizeX(), res.GetSizeX() - nDX );
	int nMinY = Max( 0, -nDY ), nMaxY = Min( res.GetSizeY(), res.GetSizeY() - nDY );
	for ( int x = nMinX; x < nMaxX; ++x )
	{
		for ( int y = nMinY; y < nMaxY; ++y )
		{
			res[y][x] = src[y+nDY][x+nDX];
		}
	}
}

static void Add( CArray2D<float> &dst, const CArray2D<float> &a, float f )
{
	ASSERT( dst.GetSizeX() == a.GetSizeX() && dst.GetSizeY() == a.GetSizeY() );
	for ( int x = 0; x < dst.GetSizeX(); ++x )
	{
		for ( int y = 0; y < dst.GetSizeY(); ++y )
			dst[y][x] += a[y][x] * f;
	}
}
*/

template<class T>
void EnlargeImage( CArray2D<T> &r, const CArray2D<T> &src, int nMargin, bool bWrapX, bool bWrapY )
{
	// enlarge tempConv
	r.SetSizes( src.GetSizeX() + nMargin * 2, src.GetSizeY() + nMargin * 2 );
	int nModX = 1000000000, nXAdd = 0, nModY = 1000000000, nYAdd = 0;
	if ( bWrapX )
	{
		nXAdd = 100 * src.GetSizeX();
		nModX = src.GetSizeX();
	}
	if ( bWrapY )
	{
		nYAdd = 100 * src.GetSizeY();
		nModY = src.GetSizeY();
	}
	for ( int y = 0; y < r.GetSizeY(); ++y )
	{
		for ( int x = 0; x < r.GetSizeX(); ++x )
		{
			int nX = ( x - nMargin + nXAdd ) % nModX;
			int nY = ( y - nMargin + nYAdd ) % nModY;
			nX = Clamp( nX, 0, src.GetSizeX() - 1 );
			nY = Clamp( nY, 0, src.GetSizeY() - 1 );
			r[y][x] = src[nY][nX];
		}
	}
}

static void Create2XSampledSource( const CArray2D<CVec4> &src, CArray2D<float> *pRes, const CVec4 &conv, 
	int nMargin, bool bWrapX, bool bWrapY )
{
	CArray2D<float> &p = *pRes;
	int nXSize = src.GetSizeX() * 2 + nMargin * 4;
	int nYSize = src.GetSizeY() * 2 + nMargin * 4;
	p.SetSizes( nXSize, nYSize );
	// copy center
	for ( int y = 0; y < src.GetSizeY(); ++y )
	{
		for ( int x = 0; x < src.GetSizeX(); ++x )
		{
			p[y*2+nMargin*2][x*2+nMargin*2] = src[y][x] * conv;
		}
	}
	// copy edges
	int nModX = 1000000000, nXAdd = 0, nModY = 1000000000, nYAdd = 0;
	if ( bWrapX )
	{
		nXAdd = 100 * src.GetSizeX();
		nModX = src.GetSizeX();
	}
	if ( bWrapY )
	{
		nYAdd = 100 * src.GetSizeY();
		nModY = src.GetSizeY();
	}
	for ( int y = 0; y < p.GetSizeY() / 2; ++y )
	{
		for ( int x = 0; x < p.GetSizeX() / 2; ++x )
		{
			int nX = ( ( x - nMargin + nXAdd ) % nModX );
			int nY = ( ( y - nMargin + nYAdd ) % nModY );
			nX = Clamp( nX, 0, src.GetSizeX() - 1 );
			nY = Clamp( nY, 0, src.GetSizeY() - 1 );
			nX += nMargin;
			nY += nMargin;
			p[y*2][x*2] = p[nY*2][nX*2];
		}
	}	
	// interpolate
	for ( int y = 0; y < p.GetSizeY() / 2 - 1; ++y )
	{
		for ( int x = 0; x < p.GetSizeX() / 2 - 1; ++x )
		{
			int nX = x * 2, nY = y * 2;
			p[nY+1][nX] = 0.5f * ( p[nY][nX] + p[nY+2][nX]);
			p[nY][nX+1] = 0.5f * ( p[nY][nX] + p[nY][nX+2]);
			p[nY+1][nX+1] = 0.25f * ( p[nY][nX] + p[nY+2][nX] + p[nY][nX+2] + p[nY+2][nX+2] );
		}
	}
}

// create pyramid function, basis of linear interpolation
static void CreateReferenceBasisFunc( CArray2D<float> *pRes, int nHalfSize )
{
	CArray2D<float> &p = *pRes;
	const int nSize = nHalfSize * 2 + 1;
	p.SetSizes( nSize, nSize );
	for ( int y = 0; y < nSize; ++y )
	{
		float fY = 1 - abs(y-nHalfSize) * ( 1.0f / nHalfSize );
		for ( int x = 0; x < nSize; ++x )
		{
			float fX = 1 - abs(x-nHalfSize) * ( 1.0f / nHalfSize );
			p[y][x] = fY * fX;
		}
	}
	//Normalize( &p );
}

/*
static void CreateFinalResolveMatrix( CArray2D<double> *pRes, int nSize )
{
	CArray2D<double> &m = *pRes;
	// short cuts

	m.SetSizes( nSize, nSize );
	m.FillZero();
	for ( int x = 0; x < nSize; ++x )
		m[x][x] = 1;
	for ( int x = 1; x < nSize; ++x )
	{
		m[x-1][x] = 0.25;
		m[x][x-1] = 0.25;
	}
	bool bRes = InvertRobust( &m );
	ASSERT( bRes );
}
*/

/*static void Mul( vector<float> *pRes, const CArray2D<double> &m, const vector<float> &s )
{
	vector<float> &r = *pRes;
	r.resize( m.GetSizeY() );
	ASSERT( m.GetSizeX() == s.size() );
	for ( int y = 0; y < r.size(); ++y )
	{
		float fRes = 0;
		for ( int x = 0; x < m.GetSizeX(); ++x )
			fRes += m[y][x] * s[x];
		r[y] = fRes;
	}
}*/

static double fResolveCoef[15] = { -0.000114508, 0.00042735, -0.00159489, 0.00595222, -0.022214, 0.0829038, -0.309401, 1.1547, -0.309401, 0.0829038, -0.022214, 0.00595222, -0.00159489, 0.00042735, -0.000114508 };
static void MulResolve( std::vector<float> *pRes, const std::vector<float> &s )
{
	std::vector<float> &r = *pRes;
	r.resize( s.size() );
	for ( int y = 0; y < r.size(); ++y )
	{
		float fRes = 0;
		int nStart = Max( -7, -y ), nFinish = Min<int>( 7, r.size() - y - 1 );
		for ( int delta = nStart; delta <= nFinish; ++delta )
			fRes += fResolveCoef[delta + 7] * s[ y + delta ];
		r[y] = fRes;
	}
}

static void FinalResolve( CArray2D<float> *pRes, const CArray2D<float> &src )
{
	CArray2D<float> t, &r = *pRes;
	//CArray2D<double> m;
	t.SetSizes( src.GetSizeX(), src.GetSizeY() );
	r.SetSizes( src.GetSizeX(), src.GetSizeY() );
	// resolve on X axis
	//CreateFinalResolveMatrix( &m, src.GetSizeX() );
	for ( int y = 0; y < src.GetSizeY(); ++y )
	{
		std::vector<float> s, st;
		s.resize( src.GetSizeX() );
		for ( int x = 0; x < src.GetSizeX(); ++x )
			s[x] = src[y][x];
		MulResolve( &st, s );//Mul( &st, m, s );
		for ( int x = 0; x < src.GetSizeX(); ++x )
			t[y][x] = st[x] * 1.5f;
	}
	// resolve on Y axis
	//CreateFinalResolveMatrix( &m, src.GetSizeY() );
	for ( int x = 0; x < src.GetSizeX(); ++x )
	{
		std::vector<float> s, st;
		s.resize( src.GetSizeY() );
		for ( int y = 0; y < src.GetSizeY(); ++y )
			s[y] = t[y][x];
		MulResolve( &st, s );//Mul( &st, m, s );
		for ( int y = 0; y < src.GetSizeY(); ++y )
			r[y][x] = st[y] * 1.5f;
	}
	for ( int x = 0; x < src.GetSizeX(); ++x )
	{
		for ( int y = 0; y < src.GetSizeY(); ++y )
			r[y][x] *= 0.0625;
	}
}

const int N_HALF_REFERENCE = 4;//8;
const int N_MARGIN = 6; // in source texels
const int N_FINAL_MARGIN = 8;
static void CalcChannel( CArray2D<float> *pRes, const CArray2D<CVec4> &src, const CVec4 &_conv, bool bWrapX, bool bWrapY )
{
	CArray2D<float> tempConv, conv, &res = *pRes, finalSrc;
	CArray2D<float> buf2, baseFunc, convFrag;
	convFrag.SetSizes( N_HALF_REFERENCE * 2 + 1, N_HALF_REFERENCE * 2 + 1 );
	CreateReferenceBasisFunc( &baseFunc, 4 );
	Create2XSampledSource( src, &buf2, _conv, N_MARGIN, bWrapX, bWrapY );
	tempConv.SetSizes( res.GetSizeX() + 2, res.GetSizeY() + 2 );
	// calc convolution
	for ( int y = 0; y < tempConv.GetSizeY(); ++y )
	{
		for ( int x = 0; x < tempConv.GetSizeX(); ++x )
		{
			int nBaseX = ( ( x - 1 ) * 2 + N_MARGIN ) * 2 + 1;
			int nBaseY = ( ( y - 1 ) * 2 + N_MARGIN ) * 2 + 1;
			for ( int nDY = -N_HALF_REFERENCE; nDY <= N_HALF_REFERENCE; ++nDY )
				for ( int nDX = -N_HALF_REFERENCE; nDX <= N_HALF_REFERENCE; ++nDX )
					convFrag[nDY + N_HALF_REFERENCE][nDX + N_HALF_REFERENCE] = buf2[nBaseY + nDY][nBaseX + nDX];
			tempConv[y][x] = Multiply( convFrag, baseFunc );
		}
	}
	EnlargeImage( finalSrc, tempConv, N_FINAL_MARGIN - 1, bWrapX, bWrapY );
	// calc result
	FinalResolve( &conv, finalSrc );
	for ( int y = 0; y < res.GetSizeY(); ++y )
	{
		for ( int x = 0; x < res.GetSizeX(); ++x )
		{
			res[y][x] = conv[y+N_FINAL_MARGIN][x+N_FINAL_MARGIN];
		}
	}
}

bool GenerateMipLevel( CArray2D<CVec4> *pDst, const CArray2D<CVec4> &src, bool bWrapX, bool bWrapY )
{
	if ( src.GetSizeX() <= 1 || src.GetSizeY() <= 1 )
		return false;
	int nXSize = src.GetSizeX() / 2;
	int nYSize = src.GetSizeY() / 2;
	pDst->SetSizes( nXSize, nYSize );
	// generate temporary array to integrate over
	CArray2D<float> channel;
	channel.SetSizes( nXSize, nYSize );
	CalcChannel( &channel, src, CVec4(1,0,0,0), bWrapX, bWrapY );
	for ( int y = 0; y < nYSize; ++y ) for ( int x = 0; x < nXSize; ++x )
		(*pDst)[y][x].r = channel[y][x];

	CalcChannel( &channel, src, CVec4(0,1,0,0), bWrapX, bWrapY );
	for ( int y = 0; y < nYSize; ++y ) for ( int x = 0; x < nXSize; ++x )
		(*pDst)[y][x].g = channel[y][x];

	CalcChannel( &channel, src, CVec4(0,0,1,0), bWrapX, bWrapY );
	for ( int y = 0; y < nYSize; ++y ) for ( int x = 0; x < nXSize; ++x )
		(*pDst)[y][x].b = channel[y][x];

	CalcChannel( &channel, src, CVec4(0,0,0,1), bWrapX, bWrapY );
	for ( int y = 0; y < nYSize; ++y ) for ( int x = 0; x < nXSize; ++x )
		(*pDst)[y][x].a = channel[y][x];
	
	return true;
}

bool GenerateMipLevelPoint( CArray2D<CVec4> *pDst, const CArray2D<CVec4> &src )
{
	if ( src.GetSizeX() <= 1 || src.GetSizeY() <= 1 )
		return false;
	int nXSize = src.GetSizeX() / 2;
	int nYSize = src.GetSizeY() / 2;
	pDst->SetSizes( nXSize, nYSize );
	for ( int y = 0; y < nYSize; ++y )
	{
		for ( int x = 0; x < nXSize; ++x )
		{
			(*pDst)[y][x] = 0.25f * ( src[y*2][x*2] + src[y*2+1][x*2] + src[y*2][x*2+1] + src[y*2+1][x*2+1] );
		}
	}
	return true;
}

void GenerateNormals( CArray2D<CVec4> *pData, const CVec4 &conv, float fMappingSize, bool bWrapX, bool bWrapY )
{
	CArray2D<float> f, fSrc;
	fSrc.SetSizes( pData->GetSizeX(), pData->GetSizeY() );
	for ( int y = 0; y < pData->GetSizeY(); ++y )
		for ( int x = 0; x < pData->GetSizeX(); ++x )
			fSrc[y][x] = (*pData)[y][x] * conv;
	EnlargeImage( f, fSrc, 1, bWrapX, bWrapY );
	float fMX = pData->GetSizeX() / ( 2 * fMappingSize );
	float fMY = pData->GetSizeY() / ( 2 * fMappingSize );
	for ( int y = 0; y < pData->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pData->GetSizeX(); ++x )
			(*pData)[y][x] = CVec4( -( f[y+1][x+2] - f[y+1][x] ) * fMX, -( f[y+2][x+1] - f[y][x+1] ) * fMY, 1, (*pData)[y][x].w );
	}
	float fScale = 127 / 255.0f, fAdd = 128 / 255.0f;
	for ( int y = 0; y < pData->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pData->GetSizeX(); ++x )
		{
			CVec4 normal( (*pData)[y][x].x, (*pData)[y][x].g, (*pData)[y][x].b, 0 );
			Normalize( &normal );
			normal *= fScale;
			normal += CVec4( fAdd, fAdd, fAdd, 0 );
			normal.w = (*pData)[y][x].w;
			(*pData)[y][x] = normal;
		}
	}
}

}

