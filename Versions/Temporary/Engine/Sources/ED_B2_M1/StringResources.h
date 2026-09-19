#pragma once

// A marker for text that was meant to move to the string tables one day; it
// is the literal itself. RCStr, the CString loader beside it, had no callers
// and went with MFC.
#define RCSTR(s)	(s)
