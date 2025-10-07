#pragma once

class CBresenham2
{
	int x1, y1;
	int x2, y2;
	int xlen, ylen, len;
	int xinc, yinc;
	int xerr, yerr;
public:
	CBresenham2( int _x1, int _y1, int _x2, int _y2 )
		: x1( _x1 ), y1( _y1 ), x2( _x2 ), y2( _y2 ),
		xlen( abs(x2 - x1) + 1 ), ylen( abs(y2 - y1) + 1 ), len( (std::max)(xlen, ylen) ),
		xinc( boost::math::sign(x2 - x1) ), yinc( boost::math::sign(y2 - y1) ),
		xerr( 0 ), yerr( 0 ) {  }
		//
		void Next()
		{
			// x component
			xerr += xlen;
			if ( xerr >= len )
				x1 += xinc, xerr -= len;
			// y component
			yerr += ylen;
			if ( yerr >= len )
				y1 += yinc, yerr -= len;
		}
		// check for line's end
		bool IsEnd() const { return (x1 == x2) && (y1 == y2); }
		// coords access
		int GetX() const { return x1; }
		int GetY() const { return y1; }
};
template <class TFunctional>
void MakeLine2( int x1, int y1, int x2, int y2, TFunctional &func )
{
	CBresenham2 line( x1, y1, x2, y2 );
	// first point
	func( line.GetX(), line.GetY() );
	// iterate line
	while ( !line.IsEnd() )
	{
		line.Next();
		func( line.GetX(), line.GetY() );
	}
}
template <class TFunctional>
void ScanLine2( int x1, int y1, int x2, int y2, TFunctional &func )
{
	CBresenham2 line( x1, y1, x2, y2 );
	// first point
	if ( func( line.GetX(), line.GetY() ) == false )
		return;
	// iterate line
	while ( !line.IsEnd() )
	{
		line.Next();
		if ( func( line.GetX(), line.GetY() ) == false )
			break;
	}
}

//*******************************************************************
//*															CBres																*
//*******************************************************************

class CBres
{
	int xerr, yerr;
	int xlen, ylen, len;
	int xinc, yinc;

	SVector dir;
	//
	void Initialize()
	{
		xerr = 0;
		yerr = 0;

		xinc = boost::math::sign( xlen );
		yinc = boost::math::sign( ylen );
		xlen = abs( xlen ) + 1;
		ylen = abs( ylen ) + 1;
		len = (std::max)( xlen, ylen );
	}

public:
	// для того, чтобы выдавал направления
	void Init( const SVector &start, const SVector &finish)
	{ 
		xlen = finish.x - start.x;
		ylen = finish.y - start.y;

		Initialize(); 
	}

	// для того, чтобы выдавал точки
	void InitPointByDirection( const SVector& start, const SVector &direction )
	{ 
		dir = start;
		xlen = direction.x;
		ylen = direction.y;

		Initialize(); 
	}

	// для того, чтобы выдавал точки
	void InitPoint( const SVector &start, const SVector &finish)
	{
		dir = start;
		xlen = finish.x - start.x;
		ylen = finish.y - start.y;

		Initialize();
	}

	//
	void MakeStep()
	{
		xerr += xlen;
		if ( xerr >= len )
			dir.x = xinc, xerr -= len;
		else
			dir.x = 0;

		yerr += ylen;
		if ( yerr >= len )
			dir.y = yinc, yerr -= len;
		else
			dir.y = 0;
	}

	void MakePointStep()
	{
		xerr += xlen;
		if ( xerr >= len )
			dir.x += xinc, xerr -= len;

		yerr += ylen;
		if ( yerr >= len )
			dir.y += yinc, yerr -= len;
	}

	const SVector& GetDirection() const { return dir; }
};

