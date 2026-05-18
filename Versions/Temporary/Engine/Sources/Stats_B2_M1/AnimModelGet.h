#pragma once

#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/DBAnimB2.h"
#include "Misc/Asserts.h"

#include <vector>

static std::vector<const NDb::SAnimB2*> GetVisObjAnimsFromModel(const NDb::SVisObj* visObj, const NDb::EAnimationType type)
{
    if (!visObj || 
        visObj->models.empty() || 
        !visObj->models[0].pModel || 
        !visObj->models[0].pModel->pSkeleton || 
        visObj->models[0].pModel->pSkeleton->animations.empty())
        return {};

    std::vector<const NDb::SAnimB2*> wanted_anims;
    wanted_anims.reserve(8);
    const auto& anims = visObj->models[0].pModel->pSkeleton->animations;

    for (size_t i = 0; i < anims.size(); i++)
    {
        const NDb::SAnimB2* real_anim = dynamic_cast_ptr<const NDb::SAnimB2*>(anims[i]);
        if (!real_anim)
            continue;
        
        if (real_anim->eType == type)
            wanted_anims.push_back(real_anim);
    }

    return wanted_anims;
};