#ifndef __SCRIPT_CAMERA_RUN_TYPES_H__
#define __SCRIPT_CAMERA_RUN_TYPES_H__
#pragma once

#include "ScriptCameraState.h"

class CScriptCameraRunTypeMnemonics : public CMnemonicsCollector<int>
{
public:
	CScriptCameraRunTypeMnemonics();
};

extern CScriptCameraRunTypeMnemonics typeScriptCameraRunTypeMnemonics;

#endif // __SCRIPT_CAMERA_RUN_TYPES_H__

