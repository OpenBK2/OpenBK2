#pragma once

class CLongObjectCreation;
class CGivenPassabilityStObject;
struct SAIObjectsUnderConstructionUpdate;
class CAILogic;
enum EActionCommand;

// stores objects under construction (that player is being ordered to build)
// untill played issues command to actually build this object
// also hold update about these objects
class CUnderConstructionObject
{
	void SendClearUpdate();

public:
	void Clear();
	void ShowUnderConstruction( EActionCommand eCommand, const CVec2 &vStart, const CVec2 &vFinish, bool bFinished, CAILogic *pAI );
	int operator&( IBinSaver &saver );
};


