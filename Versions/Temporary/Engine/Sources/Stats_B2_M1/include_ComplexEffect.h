const NDb::SEffect *GetSceneEffect() const
{ 
	if ( sceneEffects.empty() ) 
		return pSceneEffect;	
	return sceneEffects[ rand() % sceneEffects.size() ];
}

