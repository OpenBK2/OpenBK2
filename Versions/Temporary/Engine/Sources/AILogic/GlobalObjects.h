#ifndef __GLOBAL_OBJECTS_H__
#define __GLOBAL_OBJECTS_H__

#pragma ONCE

namespace NGlobalObjects
{
	void Clear();
	void Serialize( int idChunk, IBinSaver &saver );
};

#endif // __GLOBAL_OBJECTS_H__
