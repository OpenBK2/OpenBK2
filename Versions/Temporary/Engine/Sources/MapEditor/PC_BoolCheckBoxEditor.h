#pragma once

#include "PC_ItemEditor.h"


class CPCBoolCheckBoxEditor : public CPCItemEditor
{
	OBJECT_NOCOPY_METHODS( CPCBoolCheckBoxEditor );

	bool bDefaultValue;

public:
	CPCBoolCheckBoxEditor();	
	virtual ~CPCBoolCheckBoxEditor();

	//CPCItemEditor
	bool PlaceEditor( const CTRect<int> &rPlaceRect ) { return true; }
	bool ActivateEditor( CDialog *pwndActiveDialog );
	
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
	//
	void ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam ) {}
};


