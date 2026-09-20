#include "stdafx.h"
#include "RegisterEditors.h"

#include "UserDataContainer.h"
#include "System/FilePath.h"
#include "MapEditorLib/EditorFactory.h"
#include "ED_Common/WindowSimpleSharedEditor.h"

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
	for ( std::vector<SUIEditor>::const_iterator it = uiEditors.begin(); it != uiEditors.end(); ++it )
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
	for ( std::vector<SUIEditor>::const_iterator it = uiEditors.begin(); it != uiEditors.end(); ++it )
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
	// A path under the start folder, not a database name: JoinPath, or off
	// Windows the file is written to one name with a backslash in it.
	static const std::string szPath = NFile::JoinPath( "Editor", "Editors" );
	return szPath.c_str();
}

const char* CRegisterEditorsSemiAutoMagic::GetLabel() const
{
	return "Editors";
}


