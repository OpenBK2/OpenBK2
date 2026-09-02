#ifndef __SECRES_H__
#define __SECRES_H__

// Stub for Stingray Objective Toolkit's resource id header.
//
// MapEditor.rc, ED_B2_M1.rc and ED_B2.rc all open with #include
// "toolkit\secres.h", so the include has to resolve before any of them can be
// compiled at all. The real header carried the toolkit's own resource ids,
// which lived in the commercial library along with the resources themselves.
//
// Empty on purpose. None of this tree's .rc files reference a SEC_ or IDS_SEC
// identifier, so nothing is missing yet. Add ids here as rc.exe reports them
// undefined, the same way the class stubs beside this file grew.

#endif // __SECRES_H__
