#pragma once

#include "ControlConfig.h"
#include "ControlData.h"
#include "ControlSelection.h"
#include "MapEditorLib_export.h"

// funtionality															struct	algorithm
// item data expansion / storage						*
// sort by any column												*
// fast search															*
// simpe selection / expanded selection			*
// multi tree binding												*
// clipboard																*
// copy / cut / paste												*
// undo / redo															*
// drag & drop support											*
// save / load expanded / collapsed states	
// save / load columns count / width
// save / load selected / focused elements
//
struct SHTREEITEMHash
{
	std::size_t operator()( const HTREEITEM hTreeItem ) const
	{
		return std::hash<const void *>()( hTreeItem );
	}
};
//
struct SData
{
	int nColor;
	int bReadOnly;
};
//
class CControlConfig : public IControlConfig<std::string>
{
	void SetAttribute( const std::string &rID, int nAttribute, bool bSet ) {}
	bool GetAttribute( const std::string &rID, int nAttribute ) {}
	void ClearAll( int nAttribute ) {}
	const CIDSet& GetAll( int nAttribute ) {}
	//
	void SetWidth( int nWidth, int nColumnIndex ) {}
	int GetWidth( int nColumnIndex ) {}
};
//
typedef CControlData<HTREEITEM, std::string, SData, SHTREEITEMHash> CStringControlData;
typedef CControlSelection<std::string, SData> CStringControlSelection;
//
class CBaseTreeControl
{
	CStringControlData controlData;
	CStringControlSelection controlSelection;
	CControlConfig controlConfig;
};


