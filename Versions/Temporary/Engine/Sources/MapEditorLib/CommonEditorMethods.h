#pragma once

#include <cstdint>

#include "MapEditorLib_export.h"

//! Route an SEditParameters pointer to a window through the command handler.
//!
//! uintptr_t and not uint32_t. Every caller is
//! SetEditParameters/GetEditParameters in ED_B2_M1/EditorMethods.h, which pass
//! reinterpret_cast<uintptr_t>( &params ), and the receiving end casts the
//! value straight back to a pointer and dereferences it. A uint32_t here threw
//! away the top half on x64: opening a map faulted in
//! CFieldWindow::SetEditParameters reading 0x00000000843c0210, which is
//! 0x00000212843c0210 with the high word gone. ICommandHandler::HandleCommand
//! already takes uintptr_t, so this parameter was the only narrowing in the
//! chain.
MAPEDITORLIB_EXPORT bool SetGetEditParameters( uintptr_t pEditParameters, unsigned nCommandHandlerType, int nCmdID );
MAPEDITORLIB_EXPORT void CreateRefKey( std::string *pszKey, const struct SPropertyDesc *pPropertyDesc );



