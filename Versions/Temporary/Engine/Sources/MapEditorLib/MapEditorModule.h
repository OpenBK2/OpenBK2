#pragma once

typedef void (*RegisterEditorType)( const string &szName, ObjectFactoryNewFunc pfnNewFunc );
typedef void (*UnRegisterEditorType)( const string &szName );

interface IEditorModuleRegistrator
{
	virtual void RegisterTypes( const string &szType, RegisterEditorType pfnRegistrator ) const = 0;
	virtual void UnRegisterTypes( const string &szType, UnRegisterEditorType pfnUnRegistrator ) const = 0;
};

interface IEditorModule
{
	virtual void ModuleStartup() = 0;
	virtual void ModuleShutdown() = 0;
	virtual void ModuleCreate() = 0;
	virtual void ModuleDestroy() = 0;
	virtual void ModuleCreateControls() = 0;
	virtual void ModulePostCreateControls() = 0;
	virtual void ModulePreDestroyControls() = 0;
	virtual void ModuleDestroyControls() = 0;
	virtual void ModulePostCreateMainFrame() = 0;
};

typedef const IEditorModuleRegistrator* (*GETEDITORMODULEREGISTRATOR)();
typedef void (*EDITORMODULESTARTUP)();
typedef void (*EDITORMODULESHUTDOWN)();
typedef void (*EDITORMODULECREATE)();
typedef void (*EDITORMODULEDESTROY)();
typedef void (*EDITORMODULECREATECONTROLS)();
typedef void (*EDITORMODULEPOSTCREATECONTROLS)();
typedef void (*EDITORMODULEPREDESTROYCONTROLS)();
typedef void (*EDITORMODULEDESTROYCONTROLS)();
typedef void (*EDITORMODULEPOSTCREATEMAINFRAME)();


