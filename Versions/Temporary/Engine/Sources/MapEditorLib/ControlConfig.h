#pragma once


// save / load expanded / collapsed states	
// save / load columns count / width
// save / load selected / focused elements

template <class TID, class TIDHash = std::hash<TID> >
struct IControlConfig
{
	typedef std::unordered_map<TID, int> CIDSet;
	//
	virtual void SetAttribute( const TID &rID, int nAttribute, bool bSet ) = 0;
	virtual bool GetAttribute( const TID &rID, int nAttribute ) = 0;
	virtual void ClearAll( int nAttribute ) = 0;
	virtual const CIDSet& GetAll( int nAttribute ) = 0;
	//
	virtual void SetWidth( int nWidth, int nColumnIndex ) = 0;
	virtual int GetWidth( int nColumnIndex ) = 0;
};


