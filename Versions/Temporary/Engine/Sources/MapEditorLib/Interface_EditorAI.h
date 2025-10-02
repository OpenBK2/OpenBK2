#pragma once


struct IEditorAI : public CObjectBase
{
	enum { tidTypeID = 0x1418CB00 };
	//
	virtual struct ITerraAIObserver* CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY ) = 0;
};



