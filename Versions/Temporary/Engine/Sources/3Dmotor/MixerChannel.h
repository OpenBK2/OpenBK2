#pragma once

//#include "../System/Time.hpp"
#include "System/DG.h"

namespace NAnimation
{

class CMixerChannel : public CFuncBase<float>
{
	OBJECT_NOCOPY_METHODS(CMixerChannel);

private:
	ZDATA
	CDGPtr<CFuncBase<float> > pAnimatedChannel;
	CDGPtr<CFuncBase<float> > pMasterChannel;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimatedChannel); f.Add(3,&pMasterChannel); return 0; }

protected:
	CMixerChannel() : CFuncBase<float>(0.f) {}

	virtual bool NeedUpdate()
	{
		bool bMasterChanged = pMasterChannel.Refresh();
		bool bAnimatedChanged = (IsValid(pAnimatedChannel) ? pAnimatedChannel.Refresh() : false);
		return bMasterChanged || bAnimatedChanged;
	}
	virtual void Recalc()
	{
		value = pMasterChannel->GetValue();
		if ( IsValid(pAnimatedChannel) )
		{
			value *= pAnimatedChannel->GetValue();
		}
	}

public:
	CMixerChannel( CFuncBase<float> *_pMasterChannel, CFuncBase<float> *_pAnimatedChannel, float fDefaultValue = 0.f )
		: CFuncBase<float>(fDefaultValue), pMasterChannel(_pMasterChannel), pAnimatedChannel(_pAnimatedChannel)
	{
		ASSERT( IsValid(pMasterChannel) );
	}
};
}

