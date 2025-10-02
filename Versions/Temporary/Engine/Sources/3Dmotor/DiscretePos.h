#pragma once

class CFBTransform: public CObjectBase
{
	OBJECT_BASIC_METHODS(CFBTransform);
public:
	ZDATA
	SFBTransform pos;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pos); return 0; }

	CFBTransform() {}
	CFBTransform( const SFBTransform &_pos ): pos(_pos) {}
	const SFBTransform& GetValue() const { return pos; }
	void Set( const SFBTransform &_pos ) { pos = _pos; }
};

struct SDiscretePos
{
	enum 
	{
		TURN_0   = 0,
		TURN_90  = 1,
		TURN_180 = 2,
		TURN_270 = 3,
		FLIP = 4,
	};
	ZDATA
	CVec3 ptMove;
	int   nRotation;
	CObj<CFBTransform> pTransform;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&ptMove); f.Add(3,&nRotation); f.Add(4,&pTransform); return 0; }

	SDiscretePos() {}
	SDiscretePos( CFBTransform *_pTransform, const CVec3 &_ptMove, int nRotationID ) 
		: ptMove(_ptMove), nRotation(nRotationID), pTransform(_pTransform) {}
	void MakeMatrix( SFBTransform *pRes ) const;
	void MoveAndRotate( CVec3 *pPoint ) const;
	void InvMoveAndRotate( CVec3 *pPoint ) const;
	void MoveAndRotate( vector<CVec3> *pPoints ) const;
	CFBTransform* GetTransform() const;
};

inline int AngleToRotationID( int nRotation )
{
	nRotation %= 360;
	switch ( nRotation )
	{
		case 0:
			return SDiscretePos::TURN_0;
		case 90:
			return SDiscretePos::TURN_90;
		case 180:
			return SDiscretePos::TURN_180;
		case 270:
			return SDiscretePos::TURN_270;
		default:
			ASSERT( 0 );
			return SDiscretePos::FLIP;
	}
}

inline int RotationIDToAngle( int nRotationID )
{
	switch ( nRotationID )
	{
		case SDiscretePos::TURN_0:
			return 0;
		case SDiscretePos::TURN_90:
			return 90;
		case SDiscretePos::TURN_180:
			return 180;
		case SDiscretePos::TURN_270:
			return 270;
		default:
			ASSERT( 0 );
			return 45;
	}
}

inline void SDiscretePos::MoveAndRotate( CVec3 *p ) const
{
	ASSERT( p );
	switch ( nRotation )
	{
		case SDiscretePos::TURN_90:
			swap( p->x, p->y );
			FP_BITS( p->x ) ^= 0x80000000;
			break;
		case SDiscretePos::TURN_180:
			FP_BITS( p->x ) ^= 0x80000000;
			FP_BITS( p->y ) ^= 0x80000000;
			break;
		case SDiscretePos::TURN_270:
			swap( p->x, p->y );
			FP_BITS( p->y ) ^= 0x80000000;
			break;
		case SDiscretePos::FLIP:
			break;
	}
	*p += ptMove;
}

inline void SDiscretePos::MoveAndRotate( vector<CVec3> *pPoints ) const
{
	ASSERT( pPoints );
	for ( int i = 0; i < pPoints->size(); ++i )
		MoveAndRotate( &(*pPoints)[i] );
}

inline void SDiscretePos::InvMoveAndRotate( CVec3 *pPoint ) const
{
	ASSERT( pPoint );
	*pPoint -= ptMove;
	int nInvR = nRotation;
	switch ( nRotation )
	{
		case TURN_90: nInvR = TURN_270; break;
		case TURN_270: nInvR = TURN_90; break;
	}
	SDiscretePos pos( 0, VNULL3, nInvR );
	pos.MoveAndRotate( pPoint );
}

