#include "stdafx.h"

#include "EditorScene.h"

IEditorScene* EditorScene()
{
	return Singleton<IEditorScene>();
}

