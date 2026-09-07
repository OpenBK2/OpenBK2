#pragma once

struct SObjectSet;

// Put a set of database objects into the property browser and show it.
//
// Three palettes here have wanted this -- terrain tiles, map objects and VSOs
// -- and each wrote out the same fifteen lines: build a manipulator over the
// set, ask CHID_PC_DIALOG for its view, hand the manipulator over, show the
// browser, rebuild the tree. Nothing in that is a toolkit's or an editor's
// business, and a palette being ported should not be copying it a fourth time.
//
// **Not CManipulatorManager::CreateObectSetManipulator**, which is the same
// thing everywhere else in the editor and is deliberately not used here: it
// returns the object's own manipulator when the set holds exactly one, where
// the palettes have always wrapped even a single object in a CMultiManipulator.
// Whether the property browser shows the same thing either way was not
// established, so this keeps what the palettes did rather than changing three
// of them at once on the way past. Worth settling separately.
namespace NObjectProperties
{
	void Show( const SObjectSet &rObjectSet );
}
