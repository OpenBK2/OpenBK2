
#include <float.h>
#include "Win32Helper.h"

#pragma once

template <class TYPE, class TStorageType = CPtr<TYPE>, class TPosition = CVec3, class TCoeffType = float>
class CAreaMap : public CArray2D< list<TStorageType> >
{
	typedef CArray2D< list<TStorageType> > TParent;
	ZDATA_(TParent)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(TParent*)this); return 0; }
	typedef list<TStorageType> CDataList;
	typedef CArray2D<CDataList> CBaseArea;
private:
	const TCoeffType tCellSize;								// area map cell size
	// service functional
	class CObjEqualFunctional
	{
		const TYPE *pObj;
	public:
		explicit CObjEqualFunctional( const TYPE *_pObj ) : pObj( _pObj ) {  }
		bool operator()( const TStorageType &ptr ) const { return ptr == pObj; }
	};
	// add object to the (nX, nY) cell
	void AddTo( int nX, int nY, TYPE *pObj )
	{
		CDataList &data = (*this)[nY][nX];
		CDataList::const_iterator pos = find_if( data.begin(), data.end(), CObjEqualFunctional(pObj) );
		if ( pos == data.end() )
			data.push_back( pObj );
	}
	// remove object from the (nX, nY) cell
	void RemoveFrom( int nX, int nY, TYPE *pObj )
	{
		if ( (nX < 0) || (nX >= GetSizeX()) || (nY < 0) || (nY >= GetSizeY()) )
			return;
		CDataList &data = (*this)[nY][nX];
		CDataList::iterator pos = find_if( data.begin(), data.end(), CObjEqualFunctional(pObj) );
		if ( pos != data.end() )
			data.erase( pos );
	}
public:
	// diable copy constructor and assignment operator
	// no ! cannot do this because of CObjectBase
	//CAreaMap( const CAreaMap &a ) {  }
	//CAreaMap& operator=( const CAreaMap &a ) { return *this; }

	explicit CAreaMap( TCoeffType _tCellSize ) : tCellSize( _tCellSize ) {  }
	// move object to the new position.
	// if this is a new object, this will be added to the area map
	void MoveTo( TYPE *pObj, const TPosition &vNewPos )
	{
		NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );
		const TPosition vOldPos = pObj->GetPosition();
		const int nOldIndexX = Float2Int( vOldPos.x / tCellSize - 0.5f );
		const int nOldIndexY = Float2Int( vOldPos.y / tCellSize - 0.5f );
		const int nNewIndexX = Float2Int( vNewPos.x / tCellSize - 0.5f );
		const int nNewIndexY = Float2Int( vNewPos.y / tCellSize - 0.5f );
		// if we are moved to the new cell, add to the new and, then, remove from the old one
		if ( (nOldIndexX != nNewIndexX) || (nOldIndexY != nNewIndexY) )
		{
			AddTo( nNewIndexX, nNewIndexY, pObj );
			RemoveFrom( nOldIndexX, nOldIndexY, pObj );
		}
		// set new position for the object
		pObj->SetPosition( vNewPos );
	}
	// add new object
	void Add( TYPE *pObj )
	{
		const TPosition vPos = pObj->GetPosition();
		NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );
		const int nX = Float2Int( vPos.x / tCellSize - 0.5f );
		const int nY = Float2Int( vPos.y / tCellSize - 0.5f );
		AddTo( nX, nY, pObj );
	}
	// remove object from the area map
	void Remove( TYPE *pObj )
	{
		const TPosition vPos = pObj->GetPosition();
		NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );
		const int nX = Float2Int( vPos.x / tCellSize - 0.5f );
		const int nY = Float2Int( vPos.y / tCellSize - 0.5f );
		RemoveFrom( nX, nY, pObj );
	}

	// add new object to position
	void AddToPosition( TYPE *pObj, const TPosition &vPos )
	{
		NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );
		const int nX = Float2Int( vPos.x / tCellSize - 0.5f );
		const int nY = Float2Int( vPos.y / tCellSize - 0.5f );
		AddTo( nX, nY, pObj );
	}

	// remove object from positions
	void RemoveFromPosition( TYPE *pObj, const TPosition &vPos )
	{
		NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );
		const int nX = Float2Int( vPos.x / tCellSize - 0.5f );
		const int nY = Float2Int( vPos.y / tCellSize - 0.5f );
		RemoveFrom( nX, nY, pObj );
	}
	//
	const bool IsInArea( const TPosition &vPos ) const
	{
		return ( vPos.x >= 0.0f ) && ( vPos.y >= 0.0f ) &&
			(vPos.x < GetSizeX() * tCellSize ) && ( vPos.y < GetSizeY() * tCellSize );
	}
};

template <class TYPE>
class CStructAreaMapComparator
{
	const TYPE &obj;
public:
	CStructAreaMapComparator( const TYPE &_obj ) : obj( _obj ) {  }
	const bool operator()( const TYPE &obj2 ) const { return obj == obj2; }
};

template <class TYPE, class TPosition = CVec3, class TComparator = CStructAreaMapComparator<TYPE>, class TCoeffType = float>
class CStructAreaMap : public CArray2D< list<TYPE> >
{
	typedef CArray2D< list<TYPE> > TParent;
	ZDATA_(TParent)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(TParent*)this); return 0; }
	typedef list<TYPE> CDataList;
	typedef CArray2D<CDataList> CBaseArea;
private:
	const TCoeffType tCellSize;								// area map cell size
	// add object to the (nX, nY) cell
	void AddTo( int nX, int nY, const TYPE &obj )
	{
		CDataList &data = (*this)[nY][nX];
		CDataList::const_iterator pos = find_if( data.begin(), data.end(), TComparator(obj) );
		if ( pos == data.end() )
			data.push_back( obj );
	}
	// remove object from the (nX, nY) cell
	void RemoveFrom( int nX, int nY, const TYPE &obj )
	{
		if ( (nX < 0) || (nX >= GetSizeX()) || (nY < 0) || (nY >= GetSizeY()) )
			return;
		CDataList &data = (*this)[nY][nX];
		CDataList::iterator pos = find_if( data.begin(), data.end(), TComparator(obj) );
		if ( pos != data.end() )
			data.erase( pos );
	}
	// diable copy constructor and assignment operator
	CStructAreaMap( const CStructAreaMap &a ) {  }
	CStructAreaMap& operator=( const CStructAreaMap &a ) { return *this; }
public:
	explicit CStructAreaMap( TCoeffType _tCellSize ) : tCellSize( _tCellSize ) {  }
	// move object to the new position.
	// if this is a new object, this will be added to the area map
	void MoveTo( TYPE &obj, const TPosition &vNewPos )
	{
		const TPosition vOldPos = obj.GetPosition();
		const int nOldIndexX = int( vOldPos.x / tCellSize );
		const int nOldIndexY = int( vOldPos.y / tCellSize );
		const int nNewIndexX = int( vNewPos.x / tCellSize );
		const int nNewIndexY = int( vNewPos.y / tCellSize );
		// if we are moved to the new cell, add to the new and, then, remove from the old one
		if ( (nOldIndexX != nNewIndexX) || (nOldIndexY != nNewIndexY) )
		{
			AddTo( nNewIndexX, nNewIndexY, obj );
			RemoveFrom( nOldIndexX, nOldIndexY, obj );
		}
		// set new position for the object
		obj.SetPosition( vNewPos );
	}
	// add new object
	void Add( const TYPE &obj )
	{
		const TPosition vPos = obj.GetPosition();
		const int nX = int( vPos.x / tCellSize );
		const int nY = int( vPos.y / tCellSize );
		AddTo( nX, nY, obj );
	}
	// remove object from the area map
	void Remove( const TYPE &obj )
	{
		const TPosition vPos = obj.GetPosition();
		const int nX = int( vPos.x / tCellSize );
		const int nY = int( vPos.y / tCellSize );
		RemoveFrom( nX, nY, obj );
	}

	// add new object to position
	void AddToPosition( const TYPE &obj, const TPosition &vPos )
	{
		const int nX = int( vPos.x / tCellSize );
		const int nY = int( vPos.y / tCellSize );
		AddTo( nX, nY, obj );
	}

	// remove object from positions
	void RemoveFromPosition( const TYPE &obj, const TPosition &vPos )
	{
		const int nX = int( vPos.x / tCellSize );
		const int nY = int( vPos.y / tCellSize );
		RemoveFrom( nX, nY, obj );
	}
	//
	const bool IsInArea( const TPosition &vPos ) const
	{
		return ( vPos.x >= 0 ) && (vPos.y >= 0) && (vPos.x < GetSizeX()*tCellSize) && (vPos.y < GetSizeY()*tCellSize);
	}
};


