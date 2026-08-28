// Playback: one control per running clip, its clock, its speed, its looping, and
// the ease curves that weight it against whatever else is playing.
//
// M4, and the largest single block of the 54. Like the binding in Animation.cpp
// this has no prior art, and unlike the binding it carries state that only shows
// up in play: a control that eases out and frees itself, a clip that completes at
// a time set from outside, a raw local clock written back by the engine's own
// root-motion handling. Record and replay of the real DLL's calls is what pins
// those down, and it is worth building before this file is filled in rather than
// after.
//
// GrannyFreeControlOnceUnused rather than GrannyFreeControl is what the engine
// mostly calls: a control that is easing out is still being sampled, so the
// release is deferred until the last reference to it goes.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( void ) GrannyFreeControl( granny_control *Control )
{
	GR2_STUB( "Control={}", Control );
}

GR2_API( void ) GrannyFreeControlOnceUnused( granny_control *Control )
{
	GR2_STUB( "Control={}", Control );
}

GR2_API( bool ) GrannyControlIsComplete( granny_control const *Control )
{
	GR2_STUB( "Control={}", Control );
	return false;
}

GR2_API( void ) GrannyCompleteControlAt( granny_control *Control, granny_real32 AtSeconds )
{
	GR2_STUB( "Control={} AtSeconds={}", Control, AtSeconds );
}

GR2_API( void ) GrannySetControlActive( granny_control *Control, bool Active )
{
	GR2_STUB( "Control={} Active={}", Control, Active );
}

GR2_API( granny_real32 ) GrannyGetControlClampedLocalClock( granny_control *Control )
{
	GR2_STUB( "Control={}", Control );
	return 0.0f;
}

GR2_API( void ) GrannySetControlRawLocalClock( granny_control *Control, granny_real32 LocalClock )
{
	GR2_STUB( "Control={} LocalClock={}", Control, LocalClock );
}

GR2_API( granny_real32 ) GrannyGetControlDuration( granny_control const *Control )
{
	GR2_STUB( "Control={}", Control );
	return 0.0f;
}

GR2_API( granny_real32 ) GrannyGetControlDurationLeft( granny_control *Control )
{
	GR2_STUB( "Control={}", Control );
	return 0.0f;
}

GR2_API( granny_real32 ) GrannyGetControlEffectiveWeight( granny_control const *Control )
{
	GR2_STUB( "Control={}", Control );
	return 0.0f;
}

GR2_API( granny_real32 ) GrannyGetControlSpeed( granny_control const *Control )
{
	GR2_STUB( "Control={}", Control );
	return 0.0f;
}

GR2_API( void ) GrannySetControlSpeed( granny_control *Control, granny_real32 Speed )
{
	GR2_STUB( "Control={} Speed={}", Control, Speed );
}

GR2_API( void ) GrannySetControlLoopCount( granny_control *Control, granny_int32x LoopCount )
{
	GR2_STUB( "Control={} LoopCount={}", Control, LoopCount );
}

GR2_API( void ) GrannySetControlForceClampedLooping( granny_control *Control, bool Clamp )
{
	GR2_STUB( "Control={} Clamp={}", Control, Clamp );
}

GR2_API( granny_real32 ) GrannyEaseControlIn( granny_control *Control, granny_real32 Duration,
                                              bool FromCurrent )
{
	GR2_STUB( "Control={} Duration={} FromCurrent={}", Control, Duration, FromCurrent );
	return 0.0f;
}

GR2_API( granny_real32 ) GrannyEaseControlOut( granny_control *Control, granny_real32 Duration )
{
	GR2_STUB( "Control={} Duration={}", Control, Duration );
	return 0.0f;
}

GR2_API( void ) GrannySetControlEaseIn( granny_control *Control, bool EaseIn )
{
	GR2_STUB( "Control={} EaseIn={}", Control, EaseIn );
}

GR2_API( void ) GrannySetControlEaseOut( granny_control *Control, bool EaseOut )
{
	GR2_STUB( "Control={} EaseOut={}", Control, EaseOut );
}

GR2_API( void ) GrannySetControlEaseInCurve( granny_control *Control, granny_real32 StartSeconds,
                                             granny_real32 EndSeconds, granny_real32 StartValue,
                                             granny_real32 StartTangent, granny_real32 EndTangent,
                                             granny_real32 EndValue )
{
	GR2_STUB( "Control={} StartSeconds={} EndSeconds={} StartValue={} StartTangent={} "
	           "EndTangent={} EndValue={}",
	           Control, StartSeconds, EndSeconds, StartValue, StartTangent, EndTangent, EndValue );
}

GR2_API( void ) GrannySetControlEaseOutCurve( granny_control *Control, granny_real32 StartSeconds,
                                              granny_real32 EndSeconds, granny_real32 StartValue,
                                              granny_real32 StartTangent, granny_real32 EndTangent,
                                              granny_real32 EndValue )
{
	GR2_STUB( "Control={} StartSeconds={} EndSeconds={} StartValue={} StartTangent={} "
	           "EndTangent={} EndValue={}",
	           Control, StartSeconds, EndSeconds, StartValue, StartTangent, EndTangent, EndValue );
}

}
