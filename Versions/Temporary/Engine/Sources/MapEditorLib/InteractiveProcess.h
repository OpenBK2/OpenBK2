#pragma once

#include <memory>
#include <string>


// Drives an interactive child process through its standard streams: a script
// goes to the child's stdin, and its stdout is read back until a prompt marker
// says the script has run. CInteractiveMaya is the only user; it talks MEL to
// "mayabatch -prompt" and waits for Maya's "mel: " prompt between commands.
//
// The pipe handles and the process identity live in SImpl, defined in
// InteractiveProcess.cpp, rather than here. This header reaches every exporter
// through ED_Common/BasicExporter.h -> InteractiveMaya.h, so naming HANDLE and
// PROCESS_INFORMATION in it made twenty translation units that never start a
// process require a windows.h with real declarations behind those names. Out of
// sight, InteractiveProcess.cpp is the only place the platform is named, and
// giving it a second implementation later changes nothing that includes this.
class CInteractiveProcess
{
	struct SImpl;
	std::unique_ptr<SImpl> pImpl;

public:
	CInteractiveProcess( int nResponseWaitTimeout = (60 * 1000) );
	// Out of line like the constructor, and for the same reason: unique_ptr
	// needs SImpl to be a complete type to destroy it, and it is not one here.
	~CInteractiveProcess();

	bool IsStarted();

	void SetResponseTimeout( int nResponseWaitTimeout );

	bool Start( const std::string &szCommandLine, std::string *pszErrorMessage );
	bool Execute( const std::string &szScript, const std::string &szResponseEndLabel, std::string *pszOutput, std::string *pszErrorOutput, std::string *pszErrorMessage );
	bool Stop( const std::string &szQuitScript );
};
