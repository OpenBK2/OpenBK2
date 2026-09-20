#include "stdafx.h"



// theEDCommonInstance and the DllMain that set it are gone. Its comment said
// what it was for -- "для подключения ресурсов из DLL", to reach the module's
// resources -- and the module's resources are generated C++ tables now, so
// nothing had read the handle since. The DllMain existed only to fill it, and
// the #else branch that filled it with GetModuleHandle( 0 ) off a DLL build was
// the last thing in this file naming Win32.


void ManualLoadEDCommonLibrary()
{
}




