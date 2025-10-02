#pragma once

#include "GAnimation.hpp"

namespace NAnimation
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CIdentityFunc
	{
	public:
		float operator()( float fValue ) { return fValue; }
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TTransformFunc = CIdentityFunc>
	class CAnimatedChannel : public CFuncBase<float>
	{
		OBJECT_NOCOPY_METHODS(CAnimatedChannel);

	private:
		ZDATA
		CDGPtr<ISkeletonAnimator> pAnimator;
		string szChannelName;
		int nChannelID;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimator); f.Add(3,&szChannelName); f.Add(4,&nChannelID); return 0; }

		bool ChannelBinded() { return (nChannelID != INVALID_CHANNEL_ID); }

	protected:
		CAnimatedChannel() : nChannelID(INVALID_CHANNEL_ID), CFuncBase<float>(0.f) {}

		virtual bool NeedUpdate()
		{
			return pAnimator.Refresh();
		}
		virtual void Recalc()
		{
			if ( !ChannelBinded() )
			{
				nChannelID = pAnimator->GetChannelIndex( szChannelName );
			}
			value = TTransformFunc()( pAnimator->GetChannelValue( nChannelID ) );
		}

	public:
		CAnimatedChannel( ISkeletonAnimator *_pAnimator, const string &_szChannelName, float fDefaultValue = 0.f )
			: CFuncBase<float>(fDefaultValue), pAnimator(_pAnimator), szChannelName(_szChannelName), nChannelID(INVALID_CHANNEL_ID)
		{
		}
	};
}

