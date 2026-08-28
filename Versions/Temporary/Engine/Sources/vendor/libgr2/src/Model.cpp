// Model instances: one animatable copy of a model out of a file, and the clock
// that drives it.
//
// M3. A granny_model lives in the file and is shared; a granny_model_instance is
// per drawn object and owns the controls bound to it. The engine advances the
// instance clock once per frame and then samples, which is why SetModelClock has
// to be here rather than with the controls it ends up moving.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( granny_model_instance * ) GrannyInstantiateModel( granny_model const *Model )
{
	GR2_STUB( "Model={}", Model );
	return 0;
}

GR2_API( void ) GrannyFreeModelInstance( granny_model_instance *ModelInstance )
{
	GR2_STUB( "ModelInstance={}", ModelInstance );
}

GR2_API( void ) GrannySetModelClock( granny_model_instance const *ModelInstance,
                                     granny_real32 NewClock )
{
	GR2_STUB( "ModelInstance={} NewClock={}", ModelInstance, NewClock );
}

}
