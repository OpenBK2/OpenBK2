
#pragma once

// Регистрация "ручная", разрегистрация в деструкторе
class CRegisterEditorsSemiAutoMagic : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CRegisterEditorsSemiAutoMagic );

	struct SUIEditor
	{
		std::string szInstance;

		SUIEditor() {}
		SUIEditor( const std::string &_szInstance ) : 
			szInstance( _szInstance ) {}
			
		int operator&( IXmlSaver &xs );
	};
	
	std::vector<SUIEditor> uiEditors;
private:
	const char* GetXMLPath() const;
	const char* GetLabel() const;
public:
	CRegisterEditorsSemiAutoMagic();
	~CRegisterEditorsSemiAutoMagic();
	
	void Load();
	void Save();
	
	int operator&( IXmlSaver &xs );
};

extern CRegisterEditorsSemiAutoMagic g_RegisterEditorsSemiAutoMagic;


