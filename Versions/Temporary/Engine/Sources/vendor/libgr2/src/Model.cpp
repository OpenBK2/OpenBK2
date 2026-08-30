// Model instances: making one, freeing it, and moving its clock.
//
// The engine has exactly one of these per animated object.
// CSkeletonAnimator::CheckJustLoaded builds a granny_model by hand, gives it a
// skeleton borrowed from a loaded file and no mesh bindings at all, and
// instantiates that; the instance then carries the animation state that
// CSkeletonAnimator::Step advances. So an instance here is a small thing that
// grows: a model and a clock now, a list of controls when M4 arrives.
//
// Each of the three was probed against granny2.dll before being written, and the
// answers are in the comments. Two of them are deliberately not reproduced, and
// those are the two where the real DLL crashes.

#include <gr2/granny.h>

#include "Control.h"
#include "Identify.h"
#include "ModelInstance.h"
#include "Structures.h"
#include "Trace.h"

#include <new>

using namespace NGr2;

extern "C"
{

GR2_API( granny_model_instance * ) GrannyInstantiateModel( granny_model const *Model )
{
	GR2_TRACE( "Model={}", Model );

	// The real DLL access violates here, reading offset 8 of a null model, which
	// is where the skeleton pointer lives. Not reproduced: this library is fed by
	// a resource path that already crashes the game on a failed load, per
	// port/PORT_ROADMAP.md, and returning null costs one comparison.
	if ( Model == 0 )
	{
		return 0;
	}

	const SModel *pModel = reinterpret_cast<const SModel *>( Model );
	if ( pModel->pSkeleton == nullptr )
	{
		// This one the DLL does check, and it returns null. Measured, because a
		// skeleton-less model is exactly what a half-converted file would produce
		// and guessing either way would have been a coin toss.
		Logger().warn( "InstantiateModel: {}, so there is nothing to instantiate",
		               DescribeModel( pModel ) );
		return 0;
	}

	granny_model_instance *pInstance = new ( std::nothrow ) granny_model_instance;
	if ( pInstance == nullptr )
	{
		return 0;
	}

	// Referenced, not copied: measured out of the DLL through GrannyGetSourceModel,
	// which returns the address it was given, and still does after the skeleton
	// behind it changes. The caller owns the model and outlives the instance.
	pInstance->pModel = pModel;
	pInstance->fClock = 0.0f;
	return pInstance;
}

GR2_API( void ) GrannyFreeModelInstance( granny_model_instance *ModelInstance )
{
	GR2_TRACE( "ModelInstance={}", ModelInstance );

	// Null is safe in the real DLL too, and CSkeletonAnimator's destructor guards
	// it anyway.
	if ( ModelInstance != 0 )
	{
		// The controls bound to it go with it. The engine frees its instance in
		// CSkeletonAnimator's destructor and frees no controls of its own except
		// through GrannyFreeControl on a clip it drops early, so this is the only
		// place most of them are released.
		for ( size_t i = 0; i < ModelInstance->Controls.size(); ++i )
		{
			NGr2::RetireHandle( ModelInstance->Controls[i] );
			delete ModelInstance->Controls[i];
		}
	}
	NGr2::RetireHandle( ModelInstance );
	delete ModelInstance;
}

GR2_API( void ) GrannySetModelClock( granny_model_instance const *ModelInstance,
                                     granny_real32 NewClock )
{
	GR2_TRACE( "ModelInstance={} NewClock={}", ModelInstance, NewClock );

	// The real DLL access violates on a null instance. Not reproduced, same
	// reasoning as above.
	if ( ModelInstance == 0 )
	{
		return;
	}

	// const in Granny's own signature, and it plainly mutates. Granny's controls
	// hang off the instance and the clock is bookkeeping rather than content, so
	// the const is about the model tree rather than about the handle. Casting it
	// away here keeps that oddity in one place instead of spreading a mutable
	// alias through the header.
	granny_model_instance *pInstance =
		const_cast<granny_model_instance *>( ModelInstance );
	pInstance->fClock = NewClock;
}

}
