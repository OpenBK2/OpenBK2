// Measures the scancode to DIK lookup that Input's event watch does once per key
// transition: the linear scan that is there now, against a direct-indexed table
// over the same pairs.
//
// The question it settles is whether SdlScancodeToDirectInputKey in
// port/dinput.h is worth replacing, and the answer recorded in PORT_ROADMAP.md
// is that it is not: the scan costs single digit microseconds of CPU per second
// at a rate no human produces. Kept so that the number can be re-measured rather
// than re-argued, and so the direct-indexed alternative below is on hand if it
// is ever wanted for readability.
//
// The table here is a trimmed copy of the real one, same length and ordering, so
// the shape of the measurement matches. Build and run it standalone:
//
//   g++ -O2 -std=c++17 scripts/port/bench-scancode-lookup.cpp //       -o /tmp/bench $(pkg-config --cflags sdl3) && /tmp/bench
#include <SDL3/SDL.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

struct SDirectInputKey
{
	int nKey;
	SDL_Scancode scancode;
};

// the real table, trimmed to what matters for the shape of the measurement:
// same length, same ordering, same values in the positions that get hit
#define ROW( k, s ) { k, s }
constexpr SDirectInputKey directInputKeys[] = {
	ROW( 0x01, SDL_SCANCODE_ESCAPE ), ROW( 0x02, SDL_SCANCODE_1 ), ROW( 0x03, SDL_SCANCODE_2 ),
	ROW( 0x04, SDL_SCANCODE_3 ), ROW( 0x05, SDL_SCANCODE_4 ), ROW( 0x06, SDL_SCANCODE_5 ),
	ROW( 0x07, SDL_SCANCODE_6 ), ROW( 0x08, SDL_SCANCODE_7 ), ROW( 0x09, SDL_SCANCODE_8 ),
	ROW( 0x0A, SDL_SCANCODE_9 ), ROW( 0x0B, SDL_SCANCODE_0 ), ROW( 0x0C, SDL_SCANCODE_MINUS ),
	ROW( 0x0D, SDL_SCANCODE_EQUALS ), ROW( 0x0E, SDL_SCANCODE_BACKSPACE ), ROW( 0x0F, SDL_SCANCODE_TAB ),
	ROW( 0x10, SDL_SCANCODE_Q ), ROW( 0x11, SDL_SCANCODE_W ), ROW( 0x12, SDL_SCANCODE_E ),
	ROW( 0x13, SDL_SCANCODE_R ), ROW( 0x14, SDL_SCANCODE_T ), ROW( 0x15, SDL_SCANCODE_Y ),
	ROW( 0x16, SDL_SCANCODE_U ), ROW( 0x17, SDL_SCANCODE_I ), ROW( 0x18, SDL_SCANCODE_O ),
	ROW( 0x19, SDL_SCANCODE_P ), ROW( 0x1A, SDL_SCANCODE_LEFTBRACKET ), ROW( 0x1B, SDL_SCANCODE_RIGHTBRACKET ),
	ROW( 0x1C, SDL_SCANCODE_RETURN ), ROW( 0x1D, SDL_SCANCODE_LCTRL ), ROW( 0x1E, SDL_SCANCODE_A ),
	ROW( 0x1F, SDL_SCANCODE_S ), ROW( 0x20, SDL_SCANCODE_D ), ROW( 0x21, SDL_SCANCODE_F ),
	ROW( 0x22, SDL_SCANCODE_G ), ROW( 0x23, SDL_SCANCODE_H ), ROW( 0x24, SDL_SCANCODE_J ),
	ROW( 0x25, SDL_SCANCODE_K ), ROW( 0x26, SDL_SCANCODE_L ), ROW( 0x27, SDL_SCANCODE_SEMICOLON ),
	ROW( 0x28, SDL_SCANCODE_APOSTROPHE ), ROW( 0x29, SDL_SCANCODE_GRAVE ), ROW( 0x2A, SDL_SCANCODE_LSHIFT ),
	ROW( 0x2B, SDL_SCANCODE_BACKSLASH ), ROW( 0x2C, SDL_SCANCODE_Z ), ROW( 0x2D, SDL_SCANCODE_X ),
	ROW( 0x2E, SDL_SCANCODE_C ), ROW( 0x2F, SDL_SCANCODE_V ), ROW( 0x30, SDL_SCANCODE_B ),
	ROW( 0x31, SDL_SCANCODE_N ), ROW( 0x32, SDL_SCANCODE_M ), ROW( 0x33, SDL_SCANCODE_COMMA ),
	ROW( 0x34, SDL_SCANCODE_PERIOD ), ROW( 0x35, SDL_SCANCODE_SLASH ), ROW( 0x36, SDL_SCANCODE_RSHIFT ),
	ROW( 0x37, SDL_SCANCODE_KP_MULTIPLY ), ROW( 0x38, SDL_SCANCODE_LALT ), ROW( 0x39, SDL_SCANCODE_SPACE ),
	ROW( 0x3A, SDL_SCANCODE_CAPSLOCK ), ROW( 0x3B, SDL_SCANCODE_F1 ), ROW( 0x3C, SDL_SCANCODE_F2 ),
	ROW( 0x3D, SDL_SCANCODE_F3 ), ROW( 0x3E, SDL_SCANCODE_F4 ), ROW( 0x3F, SDL_SCANCODE_F5 ),
	ROW( 0x40, SDL_SCANCODE_F6 ), ROW( 0x41, SDL_SCANCODE_F7 ), ROW( 0x42, SDL_SCANCODE_F8 ),
	ROW( 0x43, SDL_SCANCODE_F9 ), ROW( 0x44, SDL_SCANCODE_F10 ), ROW( 0x45, SDL_SCANCODE_NUMLOCKCLEAR ),
	ROW( 0x46, SDL_SCANCODE_SCROLLLOCK ), ROW( 0x47, SDL_SCANCODE_KP_7 ), ROW( 0x48, SDL_SCANCODE_KP_8 ),
	ROW( 0x49, SDL_SCANCODE_KP_9 ), ROW( 0x4A, SDL_SCANCODE_KP_MINUS ), ROW( 0x4B, SDL_SCANCODE_KP_4 ),
	ROW( 0x4C, SDL_SCANCODE_KP_5 ), ROW( 0x4D, SDL_SCANCODE_KP_6 ), ROW( 0x4E, SDL_SCANCODE_KP_PLUS ),
	ROW( 0x4F, SDL_SCANCODE_KP_1 ), ROW( 0x50, SDL_SCANCODE_KP_2 ), ROW( 0x51, SDL_SCANCODE_KP_3 ),
	ROW( 0x52, SDL_SCANCODE_KP_0 ), ROW( 0x53, SDL_SCANCODE_KP_PERIOD ), ROW( 0x56, SDL_SCANCODE_NONUSBACKSLASH ),
	ROW( 0x57, SDL_SCANCODE_F11 ), ROW( 0x58, SDL_SCANCODE_F12 ), ROW( 0x64, SDL_SCANCODE_F13 ),
	ROW( 0x65, SDL_SCANCODE_F14 ), ROW( 0x66, SDL_SCANCODE_F15 ), ROW( 0x70, SDL_SCANCODE_INTERNATIONAL2 ),
	ROW( 0x73, SDL_SCANCODE_INTERNATIONAL1 ), ROW( 0x79, SDL_SCANCODE_INTERNATIONAL4 ),
	ROW( 0x7B, SDL_SCANCODE_INTERNATIONAL5 ), ROW( 0x7D, SDL_SCANCODE_INTERNATIONAL3 ),
	ROW( 0x94, SDL_SCANCODE_LANG5 ), ROW( 0x8D, SDL_SCANCODE_KP_EQUALS ),
	ROW( 0x90, SDL_SCANCODE_MEDIA_PREVIOUS_TRACK ), ROW( 0x95, SDL_SCANCODE_STOP ),
	ROW( 0x99, SDL_SCANCODE_MEDIA_NEXT_TRACK ), ROW( 0x9C, SDL_SCANCODE_KP_ENTER ),
	ROW( 0x9D, SDL_SCANCODE_RCTRL ), ROW( 0xA0, SDL_SCANCODE_MUTE ), ROW( 0xA2, SDL_SCANCODE_MEDIA_PLAY_PAUSE ),
	ROW( 0xA4, SDL_SCANCODE_MEDIA_STOP ), ROW( 0xAE, SDL_SCANCODE_VOLUMEDOWN ), ROW( 0xB0, SDL_SCANCODE_VOLUMEUP ),
	ROW( 0xB2, SDL_SCANCODE_AC_HOME ), ROW( 0xB3, SDL_SCANCODE_KP_COMMA ), ROW( 0xB5, SDL_SCANCODE_KP_DIVIDE ),
	ROW( 0xB7, SDL_SCANCODE_PRINTSCREEN ), ROW( 0xB8, SDL_SCANCODE_RALT ), ROW( 0xC5, SDL_SCANCODE_PAUSE ),
	ROW( 0xC7, SDL_SCANCODE_HOME ), ROW( 0xC8, SDL_SCANCODE_UP ), ROW( 0xC9, SDL_SCANCODE_PAGEUP ),
	ROW( 0xCB, SDL_SCANCODE_LEFT ), ROW( 0xCD, SDL_SCANCODE_RIGHT ), ROW( 0xCF, SDL_SCANCODE_END ),
	ROW( 0xD0, SDL_SCANCODE_DOWN ), ROW( 0xD1, SDL_SCANCODE_PAGEDOWN ), ROW( 0xD2, SDL_SCANCODE_INSERT ),
	ROW( 0xD3, SDL_SCANCODE_DELETE ), ROW( 0xDB, SDL_SCANCODE_LGUI ), ROW( 0xDC, SDL_SCANCODE_RGUI ),
	ROW( 0xDD, SDL_SCANCODE_APPLICATION ), ROW( 0xDE, SDL_SCANCODE_POWER ), ROW( 0xDF, SDL_SCANCODE_SLEEP ),
	ROW( 0xE3, SDL_SCANCODE_WAKE ), ROW( 0xE5, SDL_SCANCODE_AC_SEARCH ), ROW( 0xE6, SDL_SCANCODE_AC_BOOKMARKS ),
	ROW( 0xE7, SDL_SCANCODE_AC_REFRESH ), ROW( 0xE8, SDL_SCANCODE_AC_STOP ), ROW( 0xE9, SDL_SCANCODE_AC_FORWARD ),
	ROW( 0xEA, SDL_SCANCODE_AC_BACK ), ROW( 0xED, SDL_SCANCODE_MEDIA_SELECT ),
};
constexpr int COUNT = sizeof( directInputKeys ) / sizeof( directInputKeys[0] );

// what is in the tree now
__attribute__( ( noinline ) ) int ScanLinear( SDL_Scancode scancode )
{
	for ( int i = 0; i < COUNT; ++i )
	{
		if ( directInputKeys[i].scancode == scancode )
		{
			return directInputKeys[i].nKey;
		}
	}
	return 0;
}

// the alternative: the key space is dense and 512 wide, so the "hash" is the
// identity function and the table is one byte per scancode
struct SForwardTable
{
	uint8_t nKey[SDL_SCANCODE_COUNT];
	constexpr SForwardTable() : nKey()
	{
		for ( int i = 0; i < COUNT; ++i )
		{
			nKey[directInputKeys[i].scancode] = (uint8_t)directInputKeys[i].nKey;
		}
	}
};
constexpr SForwardTable forwardTable;

__attribute__( ( noinline ) ) int ScanDirect( SDL_Scancode scancode )
{
	return forwardTable.nKey[scancode];
}

template <typename F>
double Measure( const char *pszName, F fn, const std::vector<SDL_Scancode> &keys, int nRepeats )
{
	volatile int nSink = 0;
	const auto tStart = std::chrono::steady_clock::now();
	for ( int r = 0; r < nRepeats; ++r )
	{
		for ( size_t i = 0; i < keys.size(); ++i )
		{
			nSink += fn( keys[i] );
		}
	}
	const auto tEnd = std::chrono::steady_clock::now();
	const double nNs = std::chrono::duration_cast<std::chrono::nanoseconds>( tEnd - tStart ).count();
	const double nPer = nNs / ( (double)nRepeats * keys.size() );
	printf( "  %-28s %7.2f ns/lookup\n", pszName, nPer );
	return nPer;
}

int main()
{
	// what an RTS player actually presses: hotkeys, control groups, modifiers,
	// camera. All of these sit in the first third of the table.
	std::vector<SDL_Scancode> hot = {
		SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_Q,
		SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R, SDL_SCANCODE_1, SDL_SCANCODE_2,
		SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LSHIFT,
		SDL_SCANCODE_LALT, SDL_SCANCODE_SPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_ESCAPE,
	};
	// the worst case: a key the table does not carry, so the scan runs to the end
	std::vector<SDL_Scancode> miss = { SDL_SCANCODE_F20, SDL_SCANCODE_CUT, SDL_SCANCODE_UNDO };

	const int nRepeats = 2000000;
	printf( "table is %d rows, %zu bytes; direct table is %zu bytes\n\n", COUNT, sizeof( directInputKeys ),
	        sizeof( forwardTable ) );

	printf( "keys an RTS player presses:\n" );
	const double fLinearHot = Measure( "linear scan (in the tree)", ScanLinear, hot, nRepeats );
	const double fDirectHot = Measure( "direct index", ScanDirect, hot, nRepeats );

	printf( "\nworst case, a key not in the table:\n" );
	const double fLinearMiss = Measure( "linear scan (in the tree)", ScanLinear, miss, nRepeats );
	Measure( "direct index", ScanDirect, miss, nRepeats );

	// 20 transitions a second is fast typing; 100 is a player hammering hotkeys
	// with rollover. A frame at 60fps is 16.7 ms.
	printf( "\nat 100 key transitions a second, worst case every time:\n" );
	printf( "  linear scan costs %.1f us of CPU per second, %.6f%% of one core\n", fLinearMiss * 100 / 1000.0,
	        fLinearMiss * 100 / 1e7 );
	printf( "  per 16.7 ms frame, that is %.4f us out of 16700 us\n", fLinearMiss * 100 / 60 / 1000.0 );
	printf( "\nspeedup available: %.1fx hot, %.1fx worst case\n", fLinearHot / fDirectHot, fLinearMiss / fDirectHot );
	return 0;
}
