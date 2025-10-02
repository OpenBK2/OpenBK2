#ifndef __GDecalInfo_H_
#define __GDecalInfo_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace NGScene
{

struct SDecalMappingInfo
{
	CVec3 vCenter, vNormal;
	float fRadius, fRotation;
};
}
#endif
