#include "StdAfx.h"
#include "RegisterEditors.h"

#include "UserDataContainer.h"
#include "../MapEditorLib/EditorFactory.h"
#include "../ED_Common/WindowSimpleSharedEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRegisterEditorsSemiAutoMagic g_RegisterEditorsSemiAutoMagic;


int CRegisterEditorsSemiAutoMagic::SUIEditor::operator&( IXmlSaver &xs )
{
	xs.Add( "Instance", &szInstance );
	return 0;
}
int CRegisterEditorsSemiAutoMagic::operator&( IXmlSaver &xs )
{
	xs.Add( "UIEditors", &uiEditors );
	return 0;
}

// CRegisterEditorsSemiAutoMagic

CRegisterEditorsSemiAutoMagic::CRegisterEditorsSemiAutoMagic()
{
}

CRegisterEditorsSemiAutoMagic::~CRegisterEditorsSemiAutoMagic()
{
	// UI
	for ( vector<SUIEditor>::const_iterator it = uiEditors.begin(); it != uiEditors.end(); ++it )
	{
		const SUIEditor &editor = *it;
		NEditorFactory::UnRegisterEditorType( editor.szInstance );
	}
}

void CRegisterEditorsSemiAutoMagic::Load()
{
	LoadXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + GetXMLPath(), ".xml", 
		GetLabel(), *this );
		
	// UI
	for ( vector<SUIEditor>::const_iterator it = uiEditors.begin(); it != uiEditors.end(); ++it )
	{
		const SUIEditor &editor = *it;
		NEditorFactory::StartRegisterEditor();
		NEditorFactory::RegisterEditorType( editor.szInstance, &CWindowSimpleSharedEditor::NewCWindowSimpleSharedEditor );
	}
}

void CRegisterEditorsSemiAutoMagic::Save()
{
	NI_VERIFY( 0, "Creates an example. Don't use.", return );
	
	// UI (samples)
	uiEditors.push_back( SUIEditor( "WindowScreen" ) );
	uiEditors.push_back( SUIEditor( "WindowSimple" ) );
	
	SaveXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + GetXMLPath(), ".xml", 
		GetLabel(), *this );
}

const char* CRegisterEditorsSemiAutoMagic::GetXMLPath() const
{
	return "Editor\\Editors";
}

const char* CRegisterEditorsSemiAutoMagic::GetLabel() const
{
	return "Editors";
}


