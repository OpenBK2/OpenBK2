#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <cstdint>
#include <string>
#include <vector>

struct IXmlSaver;

// The bit field editor, behind a boundary that names no toolkit.
//
// A property whose editor is bit_field holds a few bytes of flags, and its
// string parameter names an XML file under the editor's folder that gives each
// flag a name:
//
//   <Fields><Item><Name>Track</Name><Value>1</Value></Item> ... </Fields>
//
// where Value is the bit, counted from bit 0 of the first byte. The property's
// browse button (PC_BinaryBitFieldEditor) opens this over those bytes, and OK
// writes the checked flags back into them.
//
// Under MFC it is CBinaryBitFieldDialog over IDD_BIT_FIELD, a check list. What
// is not drawing -- reading the names, and turning bits into checks and back --
// is here, and both dialogs use it.
namespace NBitField
{
	// Runs the dialog over the nSize bytes at pData. On OK the bytes are
	// rewritten from the checks and the answer is true; Cancel leaves them alone.
	bool Run( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize );
#endif


	// ---- the part that is not drawing ----

	struct SField
	{
		std::string szName;
		int nValue;

		SField() : nValue( -1 ) {}
		int operator&( IXmlSaver &saver );
	};

	// The names, in the file's order, which is the list's order in both dialogs.
	// False when the file cannot be read; the dialog then opens empty, as the MFC
	// one always has.
	bool LoadFields( const std::string &rszFieldsFile, std::vector<SField> *pFields );

	// Whether bit nValue is set. False for a bit outside the nSize bytes, which
	// is also what the MFC dialog showed for one.
	bool IsSet( const uint8_t *pData, int nSize, int nValue );

	// Clears the bytes and sets the bit of every checked field; rChecked is
	// indexed like rFields. A field whose bit lies outside the bytes is skipped,
	// where the MFC dialog wrote past the end of the buffer.
	//
	// Checks are matched to fields by position. The MFC dialog matched them by
	// name, so in a file that gave two fields the same name both of its checks
	// set the later one's bit; no file under Editor/BitFields does.
	void Store( const std::vector<SField> &rFields, const std::vector<bool> &rChecked, uint8_t *pData, int nSize );
}
