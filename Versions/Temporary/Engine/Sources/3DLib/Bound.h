#pragma once
template<class T>
struct SGetSelf
{
	const T& operator()( const T &a ) const { return a; }
};
struct SBoundCalcer
{
	CVec3 ptMin, ptMax;
	SBoundCalcer(): ptMin( 1e38f,1e38f,1e38f ), ptMax(-1e38f,-1e38f,-1e38f) {}
	template<class TSet,class TGetPoint>
		void LookSet( const TSet &a, TGetPoint GetPoint )
	{
		for ( TSet::const_iterator i = a.begin(); i != a.end(); ++i )
		{
			const CVec3 &p = GetPoint(*i);
			ptMin.x = (std::min)( ptMin.x, p.x );
			ptMin.y = (std::min)( ptMin.y, p.y );
			ptMin.z = (std::min)( ptMin.z, p.z );
			ptMax.x = (std::max)( ptMax.x, p.x );
			ptMax.y = (std::max)( ptMax.y, p.y );
			ptMax.z = (std::max)( ptMax.z, p.z );
		}
	}
	void Clear() { ptMin = CVec3( 1e38f,1e38f,1e38f ); ptMax = CVec3(-1e38f,-1e38f,-1e38f); }
	void Add( const CVec3 &p, float fRadius )
	{
		ptMin.x = (std::min)( ptMin.x, p.x - fRadius );
		ptMin.y = (std::min)( ptMin.y, p.y - fRadius );
		ptMin.z = (std::min)( ptMin.z, p.z - fRadius );
		ptMax.x = (std::max)( ptMax.x, p.x + fRadius );
		ptMax.y = (std::max)( ptMax.y, p.y + fRadius );
		ptMax.z = (std::max)( ptMax.z, p.z + fRadius );
	}
	void Add( const CVec3 &_p )
	{
		ptMin.x = (std::min)( ptMin.x, _p.x );
		ptMin.y = (std::min)( ptMin.y, _p.y );
		ptMin.z = (std::min)( ptMin.z, _p.z );
		ptMax.x = (std::max)( ptMax.x, _p.x );
		ptMax.y = (std::max)( ptMax.y, _p.y );
		ptMax.z = (std::max)( ptMax.z, _p.z );
	}
	void Add( const SBoundCalcer &bc )
	{
		ptMin.Minimize( bc.ptMin );
		ptMax.Maximize( bc.ptMax );
	}
	void Add( const SBound &bv )
	{
		ptMin.Minimize( bv.s.ptCenter - bv.ptHalfBox );
		ptMax.Maximize( bv.s.ptCenter + bv.ptHalfBox );
	}
	bool IsEmpty() const { return ptMin.x > ptMax.x; }
	void Make( SBound *pRes ) const
	{
		if ( IsEmpty() )
			pRes->BoxInit( CVec3(0,0,0), CVec3(0,0,0) ); 
		else
			pRes->BoxInit( ptMin, ptMax ); 
	}
	void Make( SSphere *pRes ) const
	{
		if ( IsEmpty() )
		{
			pRes->ptCenter = CVec3(0,0,0);
			pRes->fRadius = 0;
		}
		else
		{
			pRes->ptCenter = ( ptMax + ptMin ) * 0.5f;
			pRes->fRadius = fabs( ptMax - ptMin ) * 0.5f;
		}
	}
};

template<class TRes, class TSet,class TGetPoint>
inline void CalcBound( TRes *pRes, const TSet &a, TGetPoint GetPoint )
{
	SBoundCalcer b;
	b.LookSet( a, GetPoint );
	b.Make( pRes );
}

inline void UpdateBounds(CVec3 & vMin, CVec3 & vMax, const CVec3 & v) {
	vMin.x = (std::min)(vMin.x, v.x);
	vMin.y = (std::min)(vMin.y, v.y);
	vMin.z = (std::min)(vMin.z, v.z);

	vMax.x = (std::max)(vMax.x, v.x);
	vMax.y = (std::max)(vMax.y, v.y);
	vMax.z = (std::max)(vMax.z, v.z);
}

inline bool DoesIntersect( const SSphere &s, const SBound &bv )
{
	CVec3 v( bv.s.ptCenter - s.ptCenter );
	return
		fabs(v.x) - s.fRadius < bv.ptHalfBox.x &&
		fabs(v.y) - s.fRadius < bv.ptHalfBox.y &&
		fabs(v.z) - s.fRadius < bv.ptHalfBox.z;
}


