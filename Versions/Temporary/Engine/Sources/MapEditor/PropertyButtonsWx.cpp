#include "stdafx.h"

#include "PropertyButtons.h"


#include "MapEditorLib/WxColourDialog.h"

namespace NPropertyButton
{
	// The picker the model palette already opens: owned by the frame, modal over
	// it, and keeping the user's custom colours as ChooseColor kept them.
	bool PickColourWx( IWidget *pOwner, uint32_t nStart, uint32_t *pnResult )
	{
		if ( pnResult == 0 )
		{
			return false;
		}
		wxColour chosen;
		// wxColour's unsigned long constructor reads 0x00BBGGRR, and GetRGB
		// answers in the same order.
		if ( !NWxColourDialog::Pick( nullptr, pOwner, wxColour( static_cast<unsigned long>( nStart ) ), &chosen ) )
		{
			return false;
		}
		( *pnResult ) = static_cast<uint32_t>( chosen.GetRGB() );
		return true;
	}
}

