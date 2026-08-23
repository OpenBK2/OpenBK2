#pragma once

#include "GLightmapLoader.h"


namespace NGScene
{
class IRender;
struct IGScene;
class CLightmapsHolder;
class CLightmapsLoader;
class CLightmapsTempHolder;

enum ELightmapQuality : int;
CLightmapsTempHolder *CreateLightmapsTempHolder();
CLightmapsHolder *FinalMergeLightmaps( CLightmapsTempHolder *pTmpHolder );
CLightmapsHolder *CalcLightmaps( IGScene *pScene, IRender *pRender, CObjectBase *pUser, int nUserID, 
	const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder );
void ApplyLightmaps( IGScene *pScene, IRender *pRender, CObjectBase *pUser, CLightmapsHolder *pLightmaps, CLightmapsLoader * pLD  );
}

