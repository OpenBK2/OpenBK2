#pragma once


interface IEditorAI : public CObjectBase
{
	enum { tidTypeID = 0x1418CB00 };
	//
	virtual interface ITerraAIObserver* CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY ) = 0;
};



