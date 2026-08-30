// Playback: one control per running clip, its clock, its weight, and the two
// ease curves that fade it against whatever else is playing.
//
// The largest block of the 54 and the one with no prior art. Everything here was
// measured by driving granny2.dll through the sequences CSkeletonAnimator
// issues, advancing the model clock forwards only, and reading the observables
// after each step. The rules are written out at the top of Control.h, and the
// arithmetic below is the only place they are implemented.
//
// One measurement is worth repeating here because it is counter-intuitive.
// GrannyControlIsComplete is not "the clip has finished playing". A clip with a
// loop count of one, run well past its end, still reports false forever. The
// only thing that makes it true is GrannyCompleteControlAt, and then only once
// the model clock reaches the time it was given. That is what makes
// CSkeletonAnimator's Recalc loop work: it deactivates complete controls, and a
// clip with no end time is meant to hold its last frame rather than stop.
//
// GrannyFreeControlOnceUnused is what the engine mostly calls, on any clip it
// gives no end time to. Measured, it has no observable effect: the control keeps
// running, keeps its weight, and stays valid indefinitely. So it is recorded and
// otherwise ignored. Freeing it when it stopped contributing would be worse than
// wrong here, because the engine goes on calling GrannyControlIsComplete and
// GrannyGetControlClampedLocalClock through the pointer it kept.

#include <gr2/granny.h>

#include "Control.h"
#include "ModelInstance.h"
#include "Structures.h"
#include "Trace.h"

#include <cmath>
#include <cstring>
#include <new>

namespace NGr2
{

float EndlessDuration()
{
	// 0x7f0fffff, measured, and the same value whatever the animation and speed.
	const uint32_t nBits = 0x7f0fffffu;
	float fValue = 0.0f;
	memcpy( &fValue, &nBits, sizeof( fValue ) );
	return fValue;
}

namespace
{

//! One of the four ease values, as the DLL stores it.
//!
//! Asking for 0.5 gives back 0.501961, which is 128/255, and 0.7 gives 0.701961,
//! which is 179/255 and not the 178 that rounding half to even would produce. So
//! the rule is floor(v * 255 + 0.5), and it was checked over twenty-one values.
float Quantise( float fValue )
{
	if ( fValue <= 0.0f )
	{
		return 0.0f;
	}
	if ( fValue >= 1.0f )
	{
		return 1.0f;
	}
	return floorf( fValue * 255.0f + 0.5f ) / 255.0f;
}

//! A cubic Bezier over four values, in the Bernstein basis.
float Bezier( const float *pValues, float fU )
{
	const float fV = 1.0f - fU;
	return pValues[0] * fV * fV * fV + 3.0f * pValues[1] * fU * fV * fV
	       + 3.0f * pValues[2] * fU * fU * fV + pValues[3] * fU * fU * fU;
}

}

void SEaseCurve::Set( float fStart, float fEnd, float fStartValue, float fStartTangent,
                      float fEndTangent, float fEndValue )
{
	bEnabled = true;
	fStartSeconds = fStart;
	fEndSeconds = fEnd;
	Values[0] = Quantise( fStartValue );
	Values[1] = Quantise( fStartTangent );
	Values[2] = Quantise( fEndTangent );
	Values[3] = Quantise( fEndValue );
}

float SEaseCurve::At( float fTime, bool bEaseIn ) const
{
	if ( !bEnabled )
	{
		return 1.0f;
	}
	// Past the side it is not easing on, a curve is one rather than its endpoint.
	// An ease-in stops applying once it is over; an ease-out does not apply until
	// it starts.
	if ( bEaseIn && fTime > fEndSeconds )
	{
		return 1.0f;
	}
	if ( !bEaseIn && fTime < fStartSeconds )
	{
		return 1.0f;
	}

	const float fSpan = fEndSeconds - fStartSeconds;
	float fU = fSpan > 0.0f ? ( fTime - fStartSeconds ) / fSpan : 1.0f;
	// On the other side each holds its endpoint value, so the parameter clamps.
	if ( fU < 0.0f )
	{
		fU = 0.0f;
	}
	if ( fU > 1.0f )
	{
		fU = 1.0f;
	}
	return Bezier( Values, fU );
}

}

float granny_control::AnimationDuration() const
{
	return pAnimation != nullptr ? pAnimation->fDuration : 0.0f;
}

float granny_control::Duration() const
{
	if ( nLoopCount <= 0 )
	{
		return NGr2::EndlessDuration();
	}
	// The magnitude, so that a negative speed still gives a positive duration,
	// which is what the DLL returns for speed -1.
	const float fRate = fabsf( fSpeed );
	if ( fRate <= 0.0f )
	{
		return NGr2::EndlessDuration();
	}
	return static_cast<float>( nLoopCount ) * AnimationDuration() / fRate;
}

float granny_control::RawLocalClock( float fModelClock ) const
{
	const float fRaw = ( fModelClock - fStartTime ) * fSpeed;
	return fRaw > 0.0f ? fRaw : 0.0f;
}

float granny_control::ClampedLocalClock( float fModelClock ) const
{
	const float fPeriod = AnimationDuration();
	if ( fPeriod <= 0.0f )
	{
		return 0.0f;
	}

	const float fRaw = RawLocalClock( fModelClock );
	// A finite clip stops at its last frame once it has played its loops out,
	// rather than wrapping back to the start. A loop count of zero never does.
	if ( nLoopCount > 0 && fRaw >= static_cast<float>( nLoopCount ) * fPeriod )
	{
		return fPeriod;
	}
	return fmodf( fRaw, fPeriod );
}

void granny_control::LoopFlags( float fModelClock, bool *pbForwards,
                                bool *pbBackwards ) const
{
	*pbForwards = false;
	*pbBackwards = false;

	if ( nLoopCount <= 0 )
	{
		// Endless, so the curve is cyclic at both ends from the first frame on.
		*pbForwards = true;
		*pbBackwards = true;
		return;
	}

	const float fPeriod = AnimationDuration();
	if ( fPeriod <= 0.0f )
	{
		return;
	}
	// Which period this is. Past the last one the clip holds its final frame, so
	// the index stops at the last rather than running on.
	int32_t nIndex = static_cast<int32_t>( RawLocalClock( fModelClock ) / fPeriod );
	if ( nIndex > nLoopCount - 1 )
	{
		nIndex = nLoopCount - 1;
	}
	*pbBackwards = nIndex > 0;
	*pbForwards = nIndex < nLoopCount - 1;
}

float granny_control::EffectiveWeight( float fModelClock ) const
{
	if ( !bActive )
	{
		return 0.0f;
	}
	return EaseIn.At( fModelClock, true ) * EaseOut.At( fModelClock, false );
}

bool granny_control::IsComplete( float fModelClock ) const
{
	// Only CompleteControlAt can make this true. A clip that has merely run out
	// of loops is not complete, which was measured over four periods.
	return bCompleteAtSet && fModelClock >= fCompleteAt;
}

extern "C"
{

GR2_API( void ) GrannyFreeControl( granny_control *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 )
	{
		return;
	}
	// Unbind it first, or the next sample walks a dangling pointer and freeing
	// the instance frees it twice.
	if ( Control->pTarget != nullptr )
	{
		std::vector<granny_control *> &controls = Control->pTarget->Controls;
		for ( size_t i = 0; i < controls.size(); ++i )
		{
			if ( controls[i] == Control )
			{
				controls.erase( controls.begin() + static_cast<ptrdiff_t>( i ) );
				break;
			}
		}
	}
	NGr2::RetireHandle( Control );
	delete Control;
}

GR2_API( void ) GrannyFreeControlOnceUnused( granny_control *Control )
{
	GR2_TRACE( "Control={}", Control );

	// Recorded and otherwise ignored. See the note at the top of this file: the
	// engine calls this on every clip it gives no end time to and then keeps
	// using the pointer, and the real DLL keeps the control alive and running.
	if ( Control != 0 )
	{
		Control->bFreeOnceUnused = true;
	}
}

GR2_API( bool ) GrannyControlIsComplete( granny_control const *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return false;
	}
	return Control->IsComplete( Control->pTarget->fClock );
}

GR2_API( void ) GrannyCompleteControlAt( granny_control *Control, granny_real32 AtSeconds )
{
	GR2_TRACE( "Control={} AtSeconds={}", Control, AtSeconds );

	if ( Control == 0 )
	{
		return;
	}
	Control->bCompleteAtSet = true;
	Control->fCompleteAt = AtSeconds;
}

GR2_API( void ) GrannySetControlActive( granny_control *Control, bool Active )
{
	GR2_TRACE( "Control={} Active={}", Control, Active );

	if ( Control != 0 )
	{
		Control->bActive = Active;
	}
}

GR2_API( granny_real32 ) GrannyGetControlClampedLocalClock( granny_control *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return 0.0f;
	}
	return Control->ClampedLocalClock( Control->pTarget->fClock );
}

GR2_API( void ) GrannySetControlRawLocalClock( granny_control *Control,
                                               granny_real32 LocalClock )
{
	GR2_TRACE( "Control={} LocalClock={}", Control, LocalClock );

	if ( Control == 0 || Control->pTarget == nullptr || Control->fSpeed == 0.0f )
	{
		return;
	}
	// Move the start time so that right now the local clock reads this, and let
	// it carry on from there. Measured: setting it to 0.658 at a model clock of
	// 1.0 and advancing to 1.5 gives 1.158, so the offset persists rather than
	// the clock being pinned.
	Control->fStartTime = Control->pTarget->fClock - LocalClock / Control->fSpeed;
}

GR2_API( granny_real32 ) GrannyGetControlDuration( granny_control const *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 )
	{
		return 0.0f;
	}
	return Control->Duration();
}

GR2_API( granny_real32 ) GrannyGetControlDurationLeft( granny_control *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return 0.0f;
	}
	if ( Control->nLoopCount <= 0 )
	{
		return NGr2::EndlessDuration();
	}
	// Model time, not local time, and it goes negative once the clip is over.
	return Control->Duration() - ( Control->pTarget->fClock - Control->fStartTime );
}

GR2_API( granny_real32 ) GrannyGetControlEffectiveWeight( granny_control const *Control )
{
	GR2_TRACE( "Control={}", Control );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return 0.0f;
	}
	return Control->EffectiveWeight( Control->pTarget->fClock );
}

GR2_API( granny_real32 ) GrannyGetControlSpeed( granny_control const *Control )
{
	GR2_TRACE( "Control={}", Control );
	return Control == 0 ? 0.0f : Control->fSpeed;
}

GR2_API( void ) GrannySetControlSpeed( granny_control *Control, granny_real32 Speed )
{
	GR2_TRACE( "Control={} Speed={}", Control, Speed );

	if ( Control == 0 )
	{
		return;
	}
	Control->fSpeed = Speed;
}

GR2_API( void ) GrannySetControlLoopCount( granny_control *Control,
                                           granny_int32x LoopCount )
{
	GR2_TRACE( "Control={} LoopCount={}", Control, LoopCount );

	if ( Control != 0 )
	{
		Control->nLoopCount = static_cast<int32_t>( LoopCount );
	}
}

GR2_API( void ) GrannySetControlForceClampedLooping( granny_control *Control, bool Clamp )
{
	GR2_TRACE( "Control={} Clamp={}", Control, Clamp );

	// Recorded, and it changes nothing measurable: the clock behaves the same
	// with it set and clear, at every loop count tried. The engine sets it on
	// every clip it starts, so if it did anything it would do it everywhere.
	if ( Control != 0 )
	{
		Control->bForceClampedLooping = Clamp;
	}
}

GR2_API( granny_real32 ) GrannyEaseControlIn( granny_control *Control,
                                              granny_real32 Duration, bool FromCurrent )
{
	GR2_TRACE( "Control={} Duration={} FromCurrent={}", Control, Duration, FromCurrent );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return 0.0f;
	}
	// A ramp from here to here plus Duration, with the same four numbers the
	// engine passes to SetControlEaseInCurve, which come out as 3u^2-2u^3.
	const float fNow = Control->pTarget->fClock;
	const float fFrom = FromCurrent ? Control->EffectiveWeight( fNow ) : 0.0f;
	Control->EaseIn.Set( fNow, fNow + Duration, fFrom, fFrom, 1.0f, 1.0f );
	return Duration;
}

GR2_API( granny_real32 ) GrannyEaseControlOut( granny_control *Control,
                                               granny_real32 Duration )
{
	GR2_TRACE( "Control={} Duration={}", Control, Duration );

	if ( Control == 0 || Control->pTarget == nullptr )
	{
		return 0.0f;
	}
	const float fNow = Control->pTarget->fClock;
	Control->EaseOut.Set( fNow, fNow + Duration, 1.0f, 1.0f, 0.0f, 0.0f );
	return Duration;
}

GR2_API( void ) GrannySetControlEaseIn( granny_control *Control, bool EaseIn )
{
	GR2_TRACE( "Control={} EaseIn={}", Control, EaseIn );

	if ( Control != 0 )
	{
		Control->EaseIn.bEnabled = EaseIn;
	}
}

GR2_API( void ) GrannySetControlEaseOut( granny_control *Control, bool EaseOut )
{
	GR2_TRACE( "Control={} EaseOut={}", Control, EaseOut );

	if ( Control != 0 )
	{
		Control->EaseOut.bEnabled = EaseOut;
	}
}

GR2_API( void ) GrannySetControlEaseInCurve( granny_control *Control,
                                             granny_real32 StartSeconds,
                                             granny_real32 EndSeconds,
                                             granny_real32 StartValue,
                                             granny_real32 StartTangent,
                                             granny_real32 EndTangent,
                                             granny_real32 EndValue )
{
	GR2_TRACE( "Control={} StartSeconds={} EndSeconds={} StartValue={} StartTangent={} "
	           "EndTangent={} EndValue={}",
	           Control, StartSeconds, EndSeconds, StartValue, StartTangent, EndTangent,
	           EndValue );

	if ( Control != 0 )
	{
		Control->EaseIn.Set( StartSeconds, EndSeconds, StartValue, StartTangent,
		                     EndTangent, EndValue );
	}
}

GR2_API( void ) GrannySetControlEaseOutCurve( granny_control *Control,
                                              granny_real32 StartSeconds,
                                              granny_real32 EndSeconds,
                                              granny_real32 StartValue,
                                              granny_real32 StartTangent,
                                              granny_real32 EndTangent,
                                              granny_real32 EndValue )
{
	GR2_TRACE( "Control={} StartSeconds={} EndSeconds={} StartValue={} StartTangent={} "
	           "EndTangent={} EndValue={}",
	           Control, StartSeconds, EndSeconds, StartValue, StartTangent, EndTangent,
	           EndValue );

	if ( Control != 0 )
	{
		Control->EaseOut.Set( StartSeconds, EndSeconds, StartValue, StartTangent,
		                      EndTangent, EndValue );
	}
}

}
