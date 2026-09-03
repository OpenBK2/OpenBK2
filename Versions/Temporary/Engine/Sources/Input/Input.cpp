#include "stdafx.h"

#include "Input/Input.h"
#include "Input/GameMessage.h"

#include <cstring>
#include "Misc/StrProc.h"

#include "port/debugging.h"
#include "port/dinput.h"
#include "port/time.h"

#include <cstdint>

#include <fmt/format.h>

#if BOOST_OS_WINDOWS
#include "Misc/Win32Helper.h"
#else
#include "System/SdlVideo.h"

#include "port/window.h"

#include <SDL3/SDL.h>

#include <cstddef>
#include <mutex>
#endif

static const int POV_RANGE_VALUE = 1000;
static const int AXIS_RANGE_VALUE = 10000;
static const int SAMPLE_BUFFER_SIZE = 1024;
// WHEEL_DELTA: what DIPROP_GRANULARITY reports for the mouse wheel, and what one
// notch is worth. Every bindconfigure of MOUSE_AXIS_Z is scaled against it.
static const int MOUSE_WHEEL_GRANULARITY = 120;
// as many buttons as SInputDataFormat below has room for
static const int JOYSTICK_BUTTON_COUNT = 32;
const uint32_t TIME_DIFF_DBL_CLK = 500;
// DDSSOOOO
#define INPUT_KEYID( vID, vOFFS )								( ( ( vID & 0xFF ) << 24 ) | ( vOFFS ) )
#define INPUT_KEYIDEX( vID, vOFFS, vSPECIAL )		( ( ( vID & 0xFF ) << 24 ) | ( ( vSPECIAL & 0xFF ) << 16 ) | ( vOFFS ) )
#define INPUT_GETACTIONOFFS( vKeyID )						( ( vKeyID ) & 0xFFFF )
#define INPUT_GETACTIONDEVICEID( vKeyID )				( ( vKeyID ) >> 24 )

namespace NInput
{

//
// вспомогательные структуры данных
//

struct SKeyInfo
{
	const char *pszName;
	int nDevType;
	int nDevAction;
	EControlType cType;
};
const SKeyInfo kiKeyInfoList [] = 
{
////// KEYBOARD //////
	{ "ESC",								DI8DEVTYPE_KEYBOARD,	DIK_ESCAPE			, CT_KEY		},
	{ "1",									DI8DEVTYPE_KEYBOARD,	DIK_1						, CT_KEY		},
	{ "2",									DI8DEVTYPE_KEYBOARD,	DIK_2						, CT_KEY		},
	{ "3",									DI8DEVTYPE_KEYBOARD,	DIK_3						, CT_KEY		},
	{ "4",									DI8DEVTYPE_KEYBOARD,	DIK_4						, CT_KEY		},
	{ "5",									DI8DEVTYPE_KEYBOARD,	DIK_5						, CT_KEY		},
	{ "6",									DI8DEVTYPE_KEYBOARD,	DIK_6						, CT_KEY		},
	{ "7",									DI8DEVTYPE_KEYBOARD,	DIK_7						, CT_KEY		},
	{ "8",									DI8DEVTYPE_KEYBOARD,	DIK_8						, CT_KEY		},
	{ "9",									DI8DEVTYPE_KEYBOARD,	DIK_9						, CT_KEY		},
	{ "0",									DI8DEVTYPE_KEYBOARD,	DIK_0						, CT_KEY		},
	{ "-",									DI8DEVTYPE_KEYBOARD,	DIK_MINUS				, CT_KEY		},
	{ "=",									DI8DEVTYPE_KEYBOARD,	DIK_EQUALS			, CT_KEY		},
	{ "BACKSPACE",					DI8DEVTYPE_KEYBOARD,	DIK_BACK				, CT_KEY		},
	{ "TAB",								DI8DEVTYPE_KEYBOARD,	DIK_TAB					, CT_KEY		},
	{ "Q",									DI8DEVTYPE_KEYBOARD,	DIK_Q						, CT_KEY		},
	{ "W",									DI8DEVTYPE_KEYBOARD,	DIK_W						, CT_KEY		},
	{ "E",									DI8DEVTYPE_KEYBOARD,	DIK_E						, CT_KEY		},
	{ "R",									DI8DEVTYPE_KEYBOARD,	DIK_R						, CT_KEY		},
	{ "T",									DI8DEVTYPE_KEYBOARD,	DIK_T						, CT_KEY		},
	{ "Y",									DI8DEVTYPE_KEYBOARD,	DIK_Y						, CT_KEY		},
	{ "U",									DI8DEVTYPE_KEYBOARD,	DIK_U						, CT_KEY		},
	{ "I",									DI8DEVTYPE_KEYBOARD,	DIK_I						, CT_KEY		},
	{ "O",									DI8DEVTYPE_KEYBOARD,	DIK_O						, CT_KEY		},
	{ "P",									DI8DEVTYPE_KEYBOARD,	DIK_P						, CT_KEY		},
	{ "[",									DI8DEVTYPE_KEYBOARD,	DIK_LBRACKET		, CT_KEY		},
	{ "]",									DI8DEVTYPE_KEYBOARD,	DIK_RBRACKET		, CT_KEY		},
	{ "ENTER",							DI8DEVTYPE_KEYBOARD,	DIK_RETURN			, CT_KEY		},
	{ "LCTRL",							DI8DEVTYPE_KEYBOARD,	DIK_LCONTROL		, CT_KEY		},
	{ "A",									DI8DEVTYPE_KEYBOARD,	DIK_A						, CT_KEY		},
	{ "S",									DI8DEVTYPE_KEYBOARD,	DIK_S						, CT_KEY		},
	{ "D",									DI8DEVTYPE_KEYBOARD,	DIK_D						, CT_KEY		},
	{ "F",									DI8DEVTYPE_KEYBOARD,	DIK_F						, CT_KEY		},
	{ "G",									DI8DEVTYPE_KEYBOARD,	DIK_G						, CT_KEY		},
	{ "H",									DI8DEVTYPE_KEYBOARD,	DIK_H						, CT_KEY		},
	{ "J",									DI8DEVTYPE_KEYBOARD,	DIK_J						, CT_KEY		},
	{ "K",									DI8DEVTYPE_KEYBOARD,	DIK_K						, CT_KEY		},
	{ "L",									DI8DEVTYPE_KEYBOARD,	DIK_L						, CT_KEY		},
	{ ";",									DI8DEVTYPE_KEYBOARD,	DIK_SEMICOLON		, CT_KEY		},
	{ "'",									DI8DEVTYPE_KEYBOARD,	DIK_APOSTROPHE	, CT_KEY		},
	{ "`",									DI8DEVTYPE_KEYBOARD,	DIK_GRAVE       , CT_KEY		},
	{ "LSHIFT",							DI8DEVTYPE_KEYBOARD,	DIK_LSHIFT      , CT_KEY		},
	{ "\\",									DI8DEVTYPE_KEYBOARD,	DIK_BACKSLASH   , CT_KEY		},
	{ "Z",									DI8DEVTYPE_KEYBOARD,	DIK_Z           , CT_KEY		},
	{ "X",									DI8DEVTYPE_KEYBOARD,	DIK_X           , CT_KEY		},
	{ "C",									DI8DEVTYPE_KEYBOARD,	DIK_C           , CT_KEY		},
	{ "V",									DI8DEVTYPE_KEYBOARD,	DIK_V           , CT_KEY		},
	{ "B",									DI8DEVTYPE_KEYBOARD,	DIK_B           , CT_KEY		},
	{ "N",									DI8DEVTYPE_KEYBOARD,	DIK_N           , CT_KEY		},
	{ "M",									DI8DEVTYPE_KEYBOARD,	DIK_M           , CT_KEY		},
	{ ",",									DI8DEVTYPE_KEYBOARD,	DIK_COMMA       , CT_KEY		},
	{ ".",									DI8DEVTYPE_KEYBOARD,	DIK_PERIOD      , CT_KEY		},
	{ "/",									DI8DEVTYPE_KEYBOARD,	DIK_SLASH       , CT_KEY		},
	{ "RSHIFT",							DI8DEVTYPE_KEYBOARD,	DIK_RSHIFT      , CT_KEY		},
	{ "NUM_MULTIPLY",				DI8DEVTYPE_KEYBOARD,	DIK_MULTIPLY    , CT_KEY		},
	{ "LALT",								DI8DEVTYPE_KEYBOARD,	DIK_LMENU       , CT_KEY		},
	{ "SPACE",							DI8DEVTYPE_KEYBOARD,	DIK_SPACE       , CT_KEY		},
	{ "CAPITAL",						DI8DEVTYPE_KEYBOARD,	DIK_CAPITAL     , CT_KEY		},
	{ "F1",									DI8DEVTYPE_KEYBOARD,	DIK_F1          , CT_KEY		},
	{ "F2",									DI8DEVTYPE_KEYBOARD,	DIK_F2          , CT_KEY		},
	{ "F3",									DI8DEVTYPE_KEYBOARD,	DIK_F3          , CT_KEY		},
	{ "F4",									DI8DEVTYPE_KEYBOARD,	DIK_F4          , CT_KEY		},
	{ "F5",									DI8DEVTYPE_KEYBOARD,	DIK_F5          , CT_KEY		},
	{ "F6",									DI8DEVTYPE_KEYBOARD,	DIK_F6          , CT_KEY		},
	{ "F7",									DI8DEVTYPE_KEYBOARD,	DIK_F7          , CT_KEY		},
	{ "F8",									DI8DEVTYPE_KEYBOARD,	DIK_F8          , CT_KEY		},
	{ "F9",									DI8DEVTYPE_KEYBOARD,	DIK_F9          , CT_KEY		},
	{ "F10",								DI8DEVTYPE_KEYBOARD,	DIK_F10         , CT_KEY		},
	{ "NUM",								DI8DEVTYPE_KEYBOARD,	DIK_NUMLOCK     , CT_KEY		},
	{ "SCROLL",							DI8DEVTYPE_KEYBOARD,	DIK_SCROLL      , CT_KEY		},
	{ "NUM_7",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD7     , CT_KEY		},
	{ "NUM_8",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD8     , CT_KEY		},
	{ "NUM_9",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD9     , CT_KEY		},
	{ "NUM_MINUS",					DI8DEVTYPE_KEYBOARD,	DIK_SUBTRACT    , CT_KEY		},
	{ "NUM_4",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD4     , CT_KEY		},
	{ "NUM_5",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD5     , CT_KEY		},
	{ "NUM_6",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD6     , CT_KEY		},
	{ "NUM_PLUS",						DI8DEVTYPE_KEYBOARD,	DIK_ADD         , CT_KEY		},
	{ "NUM_1",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD1     , CT_KEY		},
	{ "NUM_2",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD2     , CT_KEY		},
	{ "NUM_3",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD3     , CT_KEY		},
	{ "NUM_0",							DI8DEVTYPE_KEYBOARD,	DIK_NUMPAD0     , CT_KEY		},
	{ "NUM_PERIOD",					DI8DEVTYPE_KEYBOARD,	DIK_DECIMAL     , CT_KEY		},
	{ "OEM_102",						DI8DEVTYPE_KEYBOARD,	DIK_OEM_102     , CT_KEY		},
	{ "F11",								DI8DEVTYPE_KEYBOARD,	DIK_F11         , CT_KEY		},
	{ "F12",								DI8DEVTYPE_KEYBOARD,	DIK_F12         , CT_KEY		},
	{ "F13",								DI8DEVTYPE_KEYBOARD,	DIK_F13         , CT_KEY		},
	{ "F14",								DI8DEVTYPE_KEYBOARD,	DIK_F14         , CT_KEY		},
	{ "F15",								DI8DEVTYPE_KEYBOARD,	DIK_F15         , CT_KEY		},
	{ "KANA",								DI8DEVTYPE_KEYBOARD,	DIK_KANA        , CT_KEY		},
	{ "ABNT_C1",						DI8DEVTYPE_KEYBOARD,	DIK_ABNT_C1     , CT_KEY		},
	{ "CONVERT",						DI8DEVTYPE_KEYBOARD,	DIK_CONVERT     , CT_KEY		},
	{ "NOCONVERT",					DI8DEVTYPE_KEYBOARD,	DIK_NOCONVERT   , CT_KEY		},
	{ "YEN",								DI8DEVTYPE_KEYBOARD,	DIK_YEN         , CT_KEY		},
	{ "ABNT_C2",						DI8DEVTYPE_KEYBOARD,	DIK_ABNT_C2     , CT_KEY		},
	{ "NUM_EQUALS",					DI8DEVTYPE_KEYBOARD,	DIK_NUMPADEQUALS, CT_KEY		},
	{ "PREV_TRACK",					DI8DEVTYPE_KEYBOARD,	DIK_PREVTRACK   , CT_KEY		},
	{ "AT",									DI8DEVTYPE_KEYBOARD,	DIK_AT          , CT_KEY		},
	{ "COLON",							DI8DEVTYPE_KEYBOARD,	DIK_COLON       , CT_KEY		},
	{ "UNDERLINE",					DI8DEVTYPE_KEYBOARD,	DIK_UNDERLINE   , CT_KEY		},
	{ "KANJI",							DI8DEVTYPE_KEYBOARD,	DIK_KANJI       , CT_KEY		},
	{ "STOP",								DI8DEVTYPE_KEYBOARD,	DIK_STOP        , CT_KEY		},
	{ "AX",									DI8DEVTYPE_KEYBOARD,	DIK_AX          , CT_KEY		},
	{ "UNLABELED",					DI8DEVTYPE_KEYBOARD,	DIK_UNLABELED   , CT_KEY		},
	{ "NEXT_TRACK",					DI8DEVTYPE_KEYBOARD,	DIK_NEXTTRACK   , CT_KEY		},
	{ "NUM_ENTER",					DI8DEVTYPE_KEYBOARD,	DIK_NUMPADENTER , CT_KEY		},
	{ "RCTRL",							DI8DEVTYPE_KEYBOARD,	DIK_RCONTROL    , CT_KEY		},
	{ "MUTE",								DI8DEVTYPE_KEYBOARD,	DIK_MUTE        , CT_KEY		},
	{ "CALCULATOR",					DI8DEVTYPE_KEYBOARD,	DIK_CALCULATOR  , CT_KEY		},
	{ "PLAY",								DI8DEVTYPE_KEYBOARD,	DIK_PLAYPAUSE   , CT_KEY		},
	{ "MEDIA_STOP",					DI8DEVTYPE_KEYBOARD,	DIK_MEDIASTOP   , CT_KEY		},
	{ "VOL_DOWN",						DI8DEVTYPE_KEYBOARD,	DIK_VOLUMEDOWN  , CT_KEY		},
	{ "VOL_UP",							DI8DEVTYPE_KEYBOARD,	DIK_VOLUMEUP    , CT_KEY		},
	{ "WEB_HOME",						DI8DEVTYPE_KEYBOARD,	DIK_WEBHOME     , CT_KEY		},
	{ "NUM_COMMA",					DI8DEVTYPE_KEYBOARD,	DIK_NUMPADCOMMA , CT_KEY		},
	{ "NUM_DIVIDE",					DI8DEVTYPE_KEYBOARD,	DIK_DIVIDE      , CT_KEY		},
	{ "SYSRQ",							DI8DEVTYPE_KEYBOARD,	DIK_SYSRQ       , CT_KEY		},
	{ "RALT",								DI8DEVTYPE_KEYBOARD,	DIK_RMENU       , CT_KEY		},
	{ "PAUSE",							DI8DEVTYPE_KEYBOARD,	DIK_PAUSE       , CT_KEY		},
	{ "HOME",								DI8DEVTYPE_KEYBOARD,	DIK_HOME        , CT_KEY		},
	{ "UP",									DI8DEVTYPE_KEYBOARD,	DIK_UP          , CT_KEY		},
	{ "PG_UP",							DI8DEVTYPE_KEYBOARD,	DIK_PRIOR       , CT_KEY		},
	{ "LEFT",								DI8DEVTYPE_KEYBOARD,	DIK_LEFT        , CT_KEY		},
	{ "RIGHT",							DI8DEVTYPE_KEYBOARD,	DIK_RIGHT       , CT_KEY		},
	{ "END",								DI8DEVTYPE_KEYBOARD,	DIK_END         , CT_KEY		},
	{ "DOWN",								DI8DEVTYPE_KEYBOARD,	DIK_DOWN        , CT_KEY		},
	{ "PG_DOWN",						DI8DEVTYPE_KEYBOARD,	DIK_NEXT        , CT_KEY		},
	{ "INSERT",							DI8DEVTYPE_KEYBOARD,	DIK_INSERT      , CT_KEY		},
	{ "DELETE",							DI8DEVTYPE_KEYBOARD,	DIK_DELETE      , CT_KEY		},
	{ "LWIN",								DI8DEVTYPE_KEYBOARD,	DIK_LWIN        , CT_KEY		},
	{ "RWIN",								DI8DEVTYPE_KEYBOARD,	DIK_RWIN        , CT_KEY		},
	{ "APP_MENU",						DI8DEVTYPE_KEYBOARD,	DIK_APPS        , CT_KEY		},
	{ "POWER",							DI8DEVTYPE_KEYBOARD,	DIK_POWER       , CT_KEY		},
	{ "SLEEP",							DI8DEVTYPE_KEYBOARD,	DIK_SLEEP       , CT_KEY		},
	{ "WAKE",								DI8DEVTYPE_KEYBOARD,	DIK_WAKE        , CT_KEY		},
	{ "WEB_SEARCH",					DI8DEVTYPE_KEYBOARD,	DIK_WEBSEARCH   , CT_KEY		},
	{ "WEB_FAVOR",					DI8DEVTYPE_KEYBOARD,	DIK_WEBFAVORITES, CT_KEY		},
	{ "WEB_REFRESH",				DI8DEVTYPE_KEYBOARD,	DIK_WEBREFRESH  , CT_KEY		},
	{ "WEB_STOP",						DI8DEVTYPE_KEYBOARD,	DIK_WEBSTOP     , CT_KEY		},
	{ "WEB_FORWARD",				DI8DEVTYPE_KEYBOARD,	DIK_WEBFORWARD  , CT_KEY		},
	{ "WEB_BACK",						DI8DEVTYPE_KEYBOARD,	DIK_WEBBACK     , CT_KEY		},
	{ "MYCOMPUTER",					DI8DEVTYPE_KEYBOARD,	DIK_MYCOMPUTER  , CT_KEY		},
	{ "MAIL",								DI8DEVTYPE_KEYBOARD,	DIK_MAIL        , CT_KEY		},
	{ "MEDIA_SELECT",				DI8DEVTYPE_KEYBOARD,	DIK_MEDIASELECT , CT_KEY		},
////// MOUSE //////
	{ "MOUSE_AXIS_X",				DI8DEVTYPE_MOUSE,			DIMOFS_X				, CT_AXIS		},
	{ "MOUSE_AXIS_Y",				DI8DEVTYPE_MOUSE,			DIMOFS_Y				, CT_AXIS		},
	{ "MOUSE_AXIS_Z",				DI8DEVTYPE_MOUSE,			DIMOFS_Z				, CT_AXIS		},
	{ "MOUSE_BUTTON0",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON0	, CT_KEY		},
	{ "MOUSE_BUTTON1",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON1	, CT_KEY		},
	{ "MOUSE_BUTTON2",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON2	, CT_KEY		},
	{ "MOUSE_BUTTON3",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON3	, CT_KEY		},
	{ "MOUSE_BUTTON4",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON4	, CT_KEY		},
	{ "MOUSE_BUTTON5",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON5	, CT_KEY		},
	{ "MOUSE_BUTTON6",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON6	, CT_KEY		},
	{ "MOUSE_BUTTON7",			DI8DEVTYPE_MOUSE,			DIMOFS_BUTTON7	, CT_KEY		},
	
	{ "",								0 }
};
const int DIK_DBLCLK_MODIFIER = 0x4000;

struct SKey
{
	int nAction;
	int nDevType;
	uint32_t dwLastValue;
	EPOVAxis ePOVAxis;
	EControlType eType;
	uint32_t dwLastPressed;

	SKey(): ePOVAxis( PA_UNKNOWN ), nAction( 0 ), nDevType( 0 ), dwLastValue( 0xBAD ), eType( CT_UNKNOWN ), dwLastPressed(0) {}
	SKey( int _nAction, int _nDevType, EControlType _eType, EPOVAxis _ePOVAxis = PA_UNKNOWN ): dwLastValue( 0xBAD ), nAction( _nAction ), nDevType( _nDevType ), eType( _eType ), ePOVAxis( _ePOVAxis ), dwLastPressed(0)  {}
};
struct SInputEvent
{
	uint32_t dwSequence;
	SMessage sMessage;
};
struct SInputDevice
{
	int nID;
	bool bPoll;
	bool bNeedResync;
	std::string szName;
	uint32_t dwDevType;
	uint32_t dwFormatSize;
#if BOOST_OS_WINDOWS
	NWin32Helper::com_ptr<IDirectInputDevice8> pdiDevice;
#else
	// null for the keyboard and the mouse, which SDL reports without a handle
	SDL_Joystick *pJoystick;
	SDL_JoystickID joystickID;
#endif
	//
#if BOOST_OS_WINDOWS
	SInputDevice(): bPoll( false ), bNeedResync( false ), dwDevType( 0 ), pdiDevice( 0 ) {  }
#else
	SInputDevice(): bPoll( false ), bNeedResync( false ), dwDevType( 0 ), pJoystick( 0 ), joystickID( 0 ) {  }
#endif
};
#if BOOST_OS_WINDOWS
struct SInputDeviceEnum
{
	int nID;
	int nNumControls;
	std::string szName;
	std::vector<DIOBJECTDATAFORMAT> vectorObjects;

	SInputDeviceEnum(): nID( 0 ), nNumControls( 0 ), szName( "" ) {}
};
#endif
struct SInputDataFormat
{
	int32_t  lX;
	int32_t  lY;
	int32_t  lZ;
	int32_t  lRX;
	int32_t  lRY;
	int32_t  lRZ;
	int32_t  lPOV;
	uint8_t  bButton[32];
};

typedef std::vector<SInputEvent> CEventList;
typedef std::vector<SInputDevice> CDevicesList;
///
static std::unordered_map<std::string, int> nameIDs;
static std::unordered_map<uint32_t, SKey> actionIDs;
static std::unordered_map<int, std::string> idNames;
///
static int nCounter[4] = { 0, 0, 0, 0 };
static HWND hWindow = 0;
static bool bNonExclusiveMode = false;
static bool bInitialized = false;
static bool bFocusCaptured = false;
static bool bCoopLevelSet = false;
static CDevicesList devices;
#if BOOST_OS_WINDOWS
static NWin32Helper::com_ptr<IDirectInput8> pdiInput;
#endif
///
static STime sLastEventTime = 0;
static CEventList events;
static std::list<SMessage> messages;
///
static bool SetCoopLevel();
static bool SetFocus( bool bFocus );
static void ReleaseKeyboardState();
static void ResyncDevice( const SInputDevice &sDevice );
static void AddDeviceKeys( int nID, int nDevType );
static void AddDeviceControl( int nDeviceID, int nDevType, int nOfs, EControlType eType, const std::string &szName );
static std::string MakeDeviceEnumName( int nDevType );

// The device layer, which is the only part of this file that differs between
// platforms. Everything above it - the name table, the action registry, the
// double click synthesis, the POV decomposition and the message list - is shared.
//
//! Open every device the machine has and register its controls. Called once.
static bool OpenDevices( int nSampleBufferSize );
//! Let go of them all, on the way out.
static void CloseDevices();
//! Take or give up whatever claim the platform makes on the devices.
static bool AcquireDevices( bool bAcquire );
//! Whether the game's window is the one the keyboard is going to.
static bool IsWindowFocused();
//! Give the platform a chance to move what the hardware has produced into
//! whatever ReadDeviceData is going to read it out of.
static void PumpDeviceEvents();
//! The events buffered since the last call, ordered as the device produced
//! them, up to *pdwElements of them. False when the device gave nothing usable,
//! which is not an error: a device can be lost and re-acquired at any moment.
static bool ReadDeviceData( SInputDevice &sDevice, DIDEVICEOBJECTDATA *pObjects, unsigned long *pdwElements );
//! The device's state right now, laid out by control offset, which is how
//! ResyncDevice indexes it. The buffer is already sized to dwFormatSize.
static void ReadDeviceState( const SInputDevice &sDevice, std::vector<uint8_t> *pBuffer );
//! The step between two reported values of an axis. One everywhere except the
//! mouse wheel, which moves in notches.
static float ReadControlGranularity( const SKey &sKey );
//! What the driver calls this control, for the tutorial screen to show. Empty
//! when the platform will not say.
static std::string ReadControlLocalName( int nAction );

#if BOOST_OS_WINDOWS
static void AddDeviceInfo( IDirectInputDevice8 *pdiDevice, uint32_t dwFormatSize );
static void AddDeviceEnum( IDirectInputDevice8 *pdiDevice );
static BOOL CALLBACK EnumDevicesCallback( const DIDEVICEINSTANCE* pdidInstance, PVOID pContext );
static BOOL CALLBACK EnumDeviceObjectsCallback( const DIDEVICEOBJECTINSTANCE* lpdidObject, PVOID pContext );
#endif

//
// Initialization / Deinitialization / message handling
//

// Инициализировать устройства ввода
bool InitInput( HWND hWnd, bool _bNonExclusiveMode, int nSampleBufferSize )
{
	if ( bInitialized )
		return true;

	hWindow = hWnd;
	bNonExclusiveMode = _bNonExclusiveMode;

	if ( !OpenDevices( nSampleBufferSize ) )
		return false;

	bInitialized = true;
	SetFocus( true );
	
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
		ResyncDevice( *iTempDevice );

	events.resize( SAMPLE_BUFFER_SIZE * devices.size() );
	messages.clear();

	return true;
}

bool DoneInput()
{
	if ( !bInitialized )
		return true;

	SetFocus( false );
	CloseDevices();
	devices.clear();
	bInitialized = false;

	return true;
}

#if BOOST_OS_WINDOWS

// Открыть DirectInput и все его устройства
static bool OpenDevices( int nSampleBufferSize )
{
	HRESULT hRes;
	NWin32Helper::com_ptr<IDirectInputDevice8> pdiTempDevice;

	hRes = DirectInput8Create( GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)pdiInput.GetAddr(), 0 );
	if( FAILED(hRes) )
		return false;

	hRes = pdiInput->CreateDevice( GUID_SysMouse, pdiTempDevice.GetAddr(), 0 );
	if ( SUCCEEDED( hRes ) )
	{
		hRes = pdiTempDevice->SetDataFormat( &c_dfDIMouse2 );
		if( FAILED(hRes) )
			return false;

		DIPROPDWORD sProp;
		sProp.diph.dwSize       = sizeof(DIPROPDWORD);
		sProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		sProp.diph.dwObj        = 0;
		sProp.diph.dwHow        = DIPH_DEVICE;
		sProp.dwData            = DIPROPAXISMODE_ABS;
		hRes = pdiTempDevice->SetProperty( DIPROP_AXISMODE, &sProp.diph );
		if( FAILED(hRes) )
			return false;

		sProp.diph.dwSize       = sizeof(DIPROPDWORD);
		sProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		sProp.diph.dwObj        = 0;
		sProp.diph.dwHow        = DIPH_DEVICE;
		sProp.dwData            = nSampleBufferSize > 0 ? nSampleBufferSize : SAMPLE_BUFFER_SIZE;
		hRes = pdiTempDevice->SetProperty( DIPROP_BUFFERSIZE, &sProp.diph );
		if( FAILED(hRes) )
			return false;

		AddDeviceInfo( pdiTempDevice, c_dfDIMouse2.dwDataSize );
	}

	hRes = pdiInput->CreateDevice( GUID_SysKeyboard, pdiTempDevice.GetAddr(), 0 );
	if ( SUCCEEDED( hRes ) )
	{
		hRes = pdiTempDevice->SetDataFormat( &c_dfDIKeyboard );
		if ( FAILED( hRes ) )
			return false;

		DIPROPDWORD sProp;
		sProp.diph.dwSize       = sizeof(DIPROPDWORD);
		sProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		sProp.diph.dwObj        = 0;
		sProp.diph.dwHow        = DIPH_DEVICE;
		sProp.dwData            = SAMPLE_BUFFER_SIZE;
		hRes = pdiTempDevice->SetProperty( DIPROP_BUFFERSIZE, &sProp.diph );
		if( FAILED(hRes) )
			return false;
		
		AddDeviceInfo( pdiTempDevice, c_dfDIKeyboard.dwDataSize );
	}

	pdiInput->EnumDevices( DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY );

	return true;
}

static void CloseDevices()
{
	pdiInput = 0;
}

static bool SetCoopLevel()
{
	if ( bCoopLevelSet )
		return true;
	
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		HRESULT hRes;

		if ( ( is_debugger_present() && ( GET_DIDEVICE_TYPE( iTempDevice->dwDevType ) != DI8DEVTYPE_MOUSE ) ) || bNonExclusiveMode )
			hRes = iTempDevice->pdiDevice->SetCooperativeLevel( hWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND );
		else
		{
			int nDeviceID = GET_DIDEVICE_TYPE( iTempDevice->dwDevType );
			if ( nDeviceID == DI8DEVTYPE_KEYBOARD || nDeviceID == DI8DEVTYPE_MOUSE )
				hRes = iTempDevice->pdiDevice->SetCooperativeLevel( hWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND );
			else
				hRes = iTempDevice->pdiDevice->SetCooperativeLevel( hWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND );
		}

		if( FAILED( hRes ) )
			return false;
	}
	
	bCoopLevelSet = true;
	
	return true;
}

static bool ReadDeviceData( SInputDevice &sDevice, DIDEVICEOBJECTDATA *pObjects, unsigned long *pdwElements )
{
	if ( sDevice.bPoll )
	{
		if ( FAILED( sDevice.pdiDevice->Poll() ) )
			sDevice.pdiDevice->Acquire();
	}

	const HRESULT hRes = sDevice.pdiDevice->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), pObjects, pdwElements, 0 );
	if ( hRes == DI_BUFFEROVERFLOW )
		sDevice.bNeedResync = true;
	if ( SUCCEEDED( hRes ) )
	{
		// DirectInput timestamps use the GetTickCount clock, while the binding
		// accumulators use GetCurrentTimeMilliseconds. Preserve each event's age
		// when moving it to the engine clock or a newly pressed key gets credited
		// with the clocks' epoch offset and makes a slider (such as camera motion)
		// jump on its first sample.
		const uint32_t dwDirectInputNow = GetTickCount();
		const uint32_t dwEngineNow = GetCurrentTimeMilliseconds();
		for ( unsigned long nEvent = 0; nEvent < *pdwElements; ++nEvent )
		{
			pObjects[nEvent].dwTimeStamp = dwEngineNow - ( dwDirectInputNow - pObjects[nEvent].dwTimeStamp );
		}
		return true;
	}

	sDevice.pdiDevice->Acquire();
	return false;
}

static void ReadDeviceState( const SInputDevice &sDevice, std::vector<uint8_t> *pBuffer )
{
	sDevice.pdiDevice->GetDeviceState( sDevice.dwFormatSize, &( ( *pBuffer )[0] ) );
}

static float ReadControlGranularity( const SKey &sKey )
{
	for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( GET_DIDEVICE_TYPE( iTempDevice->dwDevType ) != sKey.nDevType )
			continue;

		DIPROPDWORD diProp;
		diProp.diph.dwSize = sizeof( DIPROPDWORD );
		diProp.diph.dwHeaderSize = sizeof( DIPROPHEADER );
		diProp.diph.dwHow = DIPH_BYOFFSET;
		diProp.diph.dwObj = INPUT_GETACTIONOFFS( sKey.nAction );
		HRESULT hRes = iTempDevice->pdiDevice->GetProperty( DIPROP_GRANULARITY, (DIPROPHEADER*)&diProp );
		if ( SUCCEEDED(hRes) )
			return (float)diProp.dwData;
	}

	return 1.0f;
}

static std::string ReadControlLocalName( int nAction )
{
	const SKey &sKey = actionIDs[nAction];

	for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( GET_DIDEVICE_TYPE( iTempDevice->dwDevType ) != sKey.nDevType )
			continue;
		DIDEVICEOBJECTINSTANCE objInstance;
		memset( &objInstance, 0, sizeof(objInstance) );
		objInstance.dwSize = sizeof( objInstance );
		HRESULT hRes = iTempDevice->pdiDevice->GetObjectInfo( &objInstance, INPUT_GETACTIONOFFS(nAction), DIPH_BYOFFSET );
		return SUCCEEDED(hRes) ? objInstance.tszName : "";
	}
	return "";
}

#endif // BOOST_OS_WINDOWS

// выкачать все event'ы, произошедшие с последней выкачки
struct SSeqNumberLessThenFunctional
{
	bool operator()( const SInputEvent &sEvent1, const SInputEvent &sEvent2 ) const 
	{ 
		return ( sEvent1.dwSequence < sEvent2.dwSequence ); 
	}
};

static void FillEventInfo( SInputEvent &sEvent, SKey &sKey, const DIDEVICEOBJECTDATA &did )
{
	sEvent.dwSequence = did.dwSequence;
	sEvent.sMessage.cType = sKey.eType;
	sEvent.sMessage.tTime = did.dwTimeStamp;
	sEvent.sMessage.nParam = (int)did.dwData - (int)sKey.dwLastValue;
	sEvent.sMessage.nAction = sKey.nAction;
	sEvent.sMessage.ePOVAxis = sKey.ePOVAxis;

	//					ASSERT( sKey.eType != CT_AXIS || abs( sEvent.sMessage.nParam ) < 300 );

	sKey.dwLastValue = did.dwData;
}

static uint32_t dwPrevPump;
void PumpMessages( bool bFocus )
{
	if ( !bInitialized )
		return;

	// Background simulation may keep bFocus true after the window is deactivated.
	// Input focus must still follow the window the keyboard is actually going to.
	SetFocus( bFocus && IsWindowFocused() );
	if ( !bFocusCaptured )
		return;

	uint32_t dwTest = GetCurrentTimeMilliseconds();
	if ( dwTest - dwPrevPump < 1 )
		return;
	dwPrevPump = dwTest;

	PumpDeviceEvents();

	int nNumEvents = 0;
	events.resize( SAMPLE_BUFFER_SIZE * devices.size() * 2 );
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		unsigned long dwElements = SAMPLE_BUFFER_SIZE;
		DIDEVICEOBJECTDATA didObjects[SAMPLE_BUFFER_SIZE];

		if ( ReadDeviceData( *iTempDevice, didObjects, &dwElements ) )
		{
			for ( int nTemp = 0; nTemp < dwElements; ++nTemp )
			{
				const DIDEVICEOBJECTDATA &did = didObjects[ nTemp ];
				if ( actionIDs.find( INPUT_KEYID( iTempDevice->nID, did.dwOfs ) ) != actionIDs.end() )
				{
					SKey &sKey = actionIDs[ INPUT_KEYID( iTempDevice->nID, did.dwOfs ) ];
					SInputEvent &sEvent = events[nNumEvents++];
					FillEventInfo( sEvent, sKey, didObjects[ nTemp ] );
					sEvent.sMessage.bState = true;

					if ( sKey.eType == CT_KEY )
					{
						if ( did.dwData & 0x80 )
						{
							sEvent.sMessage.bState = true;

							if ( sEvent.sMessage.tTime - sKey.dwLastPressed < TIME_DIFF_DBL_CLK )
							{
								SKey &sDblClkKey = actionIDs[ INPUT_KEYID( iTempDevice->nID, did.dwOfs | DIK_DBLCLK_MODIFIER ) ];
								SInputEvent &sDblClkEvent = events[nNumEvents++];
								FillEventInfo( sDblClkEvent, sDblClkKey, didObjects[ nTemp ] );
								sDblClkEvent.sMessage.bState = true;
								sKey.dwLastPressed = 0;
							}
							else
								sKey.dwLastPressed = sEvent.sMessage.tTime;
						}
						else
						{
							sEvent.sMessage.bState = false;

							SKey &sDblClkKey = actionIDs[ INPUT_KEYID( iTempDevice->nID, did.dwOfs | DIK_DBLCLK_MODIFIER ) ];
							if ( sDblClkKey.dwLastValue != did.dwData )
							{
								SInputEvent &sDblClkEvent = events[nNumEvents++];
								FillEventInfo( sDblClkEvent, sDblClkKey, didObjects[ nTemp ] );
								sDblClkEvent.sMessage.bState = false;
							}
						}
					}
				}
			}
		}
	}
#if !BOOST_OS_WINDOWS
	// Report one held state per press rather than once per frame. If a complete
	// click lands between two pumps, the edge traces above remain but no held
	// trace is emitted, which makes that timing visible in the log.
	static bool bMiddleMouseHeldTraced = false;
	const bool bMiddleMouseHeld = ( SDL_GetMouseState( 0, 0 ) & SDL_BUTTON_MASK( SDL_BUTTON_MIDDLE ) ) != 0;
	if ( bMiddleMouseHeld && !bMiddleMouseHeldTraced )
	{
		bMiddleMouseHeldTraced = true;
	}
	else if ( !bMiddleMouseHeld )
	{
		bMiddleMouseHeldTraced = false;
	}
#endif
	events.resize( nNumEvents );
	///
	sort( events.begin(), events.end(), SSeqNumberLessThenFunctional() );
	///
	for ( CEventList::const_iterator iTempEvent = events.begin(); iTempEvent != events.end(); ++iTempEvent )
	{
		if ( iTempEvent->sMessage.nAction != -1 )
		{
			messages.push_back( iTempEvent->sMessage );

			if ( iTempEvent->sMessage.cType == CT_POV )
			{
				const SKey &sKeyX = actionIDs[ INPUT_KEYIDEX( INPUT_GETACTIONDEVICEID( iTempEvent->sMessage.nAction ), INPUT_GETACTIONOFFS( iTempEvent->sMessage.nAction ), 1 ) ];
				SMessage sMessageX(
					sKeyX.nAction, sKeyX.ePOVAxis, sKeyX.eType,
					0, 
					true, 
					iTempEvent->sMessage.tTime );
				if ( iTempEvent->sMessage.nParam != - 1 )
					sMessageX.nParam = cos( ( (float)iTempEvent->sMessage.nParam * FP_2PI - FP_PI * 18000 ) / 36000 ) * POV_RANGE_VALUE;

				messages.push_back( sMessageX );

				const SKey &sKeyY = actionIDs[ INPUT_KEYIDEX( INPUT_GETACTIONDEVICEID( iTempEvent->sMessage.nAction ), INPUT_GETACTIONOFFS( iTempEvent->sMessage.nAction ), 2 ) ];
				SMessage sMessageY(
					sKeyY.nAction, sKeyY.ePOVAxis, sKeyY.eType,
					0, 
					true, 
					iTempEvent->sMessage.tTime );
				if ( iTempEvent->sMessage.nParam != - 1 )
					sMessageY.nParam = sin( ( (float)iTempEvent->sMessage.nParam * FP_2PI - FP_PI * 18000 ) / 36000 ) * POV_RANGE_VALUE;
				messages.push_back( sMessageY );
			}
		}
	}

	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( !iTempDevice->bNeedResync )
			continue;

		DebugTrace( "INPUT: Resync device %s\n", iTempDevice->szName.c_str() );
		iTempDevice->bNeedResync = false;
		ResyncDevice( *iTempDevice );
	}
}


static void ResyncDevice( const SInputDevice &sDevice )
{
	std::vector<uint8_t> sBuffer;
	sBuffer.resize( sDevice.dwFormatSize );

	ReadDeviceState( sDevice, &sBuffer );

	for ( std::unordered_map<uint32_t, SKey>::iterator iTemp = actionIDs.begin(); iTemp != actionIDs.end(); iTemp++ )
	{
		SKey &sKey = iTemp->second;
		if ( sKey.nDevType == GET_DIDEVICE_TYPE( sDevice.dwDevType ) )
		{
			uint32_t dwData = 0;
			if ( sKey.eType == CT_KEY && ( sKey.nAction & DIK_DBLCLK_MODIFIER ) )
				dwData = 0;
			else
			{
				uint8_t *pData = &( sBuffer[INPUT_GETACTIONOFFS( sKey.nAction )] );

				if ( sKey.eType == CT_KEY )
					dwData = *(uint8_t*)pData;
				else
					dwData = *(uint32_t*)pData;
			}

			if ( sKey.dwLastValue == dwData )
				continue;
			
			bool bState = true;
			if ( sKey.eType == CT_KEY )
			{
				if ( dwData & 0x80 )
					bState = true;
				else
					bState = false;
			}
			messages.insert( messages.end(),
				SMessage(
					sKey.nAction, sKey.ePOVAxis, sKey.eType,
					(int)dwData - (int)sKey.dwLastValue,
					bState,
					GetCurrentTimeMilliseconds() )
			);

			sKey.dwLastValue = dwData;
		}
	}
}

bool GetMessage( SMessage *pMsg )
{
	ASSERT( pMsg );
	if ( messages.empty() )
	{
		pMsg->cType = CT_TIME;
		pMsg->tTime = GetCurrentTimeMilliseconds();
		return false;
	}
	*pMsg = messages.front();
	messages.pop_front();
	sLastEventTime = pMsg->tTime;
	return true;
}

bool IsDInputDiscardableKey( const SMessage &mMsg )
{
	if ( mMsg.cType != CT_KEY )
		return false;

	if ( actionIDs.find( mMsg.nAction ) == actionIDs.end() )
		return false;
	const SKey &key = actionIDs[ mMsg.nAction ];
	if ( key.nDevType != DI8DEVTYPE_KEYBOARD )
		return false;
	///const string szName = idNames[ key.nAction ];
	//return idNames[ key.nAction ] != "ESC";
	return true;
}

STime GetLastEventTime()
{
	return sLastEventTime;
}

int GetControlID( const std::string &sCommand )
{
	if ( nameIDs.find( sCommand ) == nameIDs.end() )
		return -1;

	return nameIDs[sCommand];
}

void GetControlInfo( int nAction, EControlType *pcType, float *pfGranularity )
{
	if ( actionIDs.find( nAction ) == actionIDs.end() )
	{
		*pcType = CT_UNKNOWN;
		*pfGranularity = 1;
		return;
	}

	const SKey &sKey = actionIDs[nAction];
	*pcType = sKey.eType;
	switch( sKey.eType )
	{
		case CT_KEY:
			*pfGranularity = 1.0f;
			return;
		case CT_POV:
			*pfGranularity = POV_RANGE_VALUE;
			return;
		default:
			*pfGranularity = ReadControlGranularity( sKey );
			break;
	}

	return;
}

std::string GetControlLocalName( int nAction )
{
	if ( actionIDs.find( nAction ) == actionIDs.end() )
		return "";

	return ReadControlLocalName( nAction );
}


//
//	Internal functions
//

// получит / отдать контроль над девайсами
static bool SetFocus( bool bFocus )
{
	if ( !bInitialized )
		return false;
	if ( bFocusCaptured == bFocus )
		return true;
	if ( bFocus )
	{
		if ( !AcquireDevices( true ) )
			return false;

		for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
			ResyncDevice( *iTempDevice );
	}
	else
	{
		// Key-up events are not delivered once the window stops being the one the
		// keyboard goes to. Release every tracked keyboard key so Alt+Tab cannot
		// leave a bind active.
		ReleaseKeyboardState();

		if ( !AcquireDevices( false ) )
			return false;
	}

	bFocusCaptured = bFocus;

	return true;
}

#if BOOST_OS_WINDOWS

static bool AcquireDevices( bool bAcquire )
{
	if ( bAcquire && !SetCoopLevel() )
		return false;

	for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		const HRESULT hRes = bAcquire ? iTempDevice->pdiDevice->Acquire() : iTempDevice->pdiDevice->Unacquire();
		if ( FAILED( hRes ) )
			return false;
	}

	return true;
}

static bool IsWindowFocused()
{
	return GetForegroundWindow() == hWindow;
}

static void PumpDeviceEvents()
{
	// Nothing to do: a DirectInput device fills its own buffer whether or not
	// anyone asks, and GetDeviceData reads it straight out.
}

#endif

static void ReleaseKeyboardState()
{
	DebugTrace("Release keyboard state");
	const uint32_t dwTime = GetCurrentTimeMilliseconds();
	for ( std::unordered_map<uint32_t, SKey>::iterator iTemp = actionIDs.begin(); iTemp != actionIDs.end(); ++iTemp )
	{
		SKey &sKey = iTemp->second;
		if ( sKey.nDevType != DI8DEVTYPE_KEYBOARD || sKey.eType != CT_KEY )
			continue;

		if ( sKey.dwLastValue & 0x80 )
		{
			messages.push_back(
				SMessage(
					sKey.nAction, sKey.ePOVAxis, sKey.eType,
					-(int)sKey.dwLastValue,
					false,
					dwTime )
			);
		}

		sKey.dwLastValue = 0;
		sKey.dwLastPressed = 0;
	}
}

#if BOOST_OS_WINDOWS

// добавить информацию про девайс
static void AddDeviceInfo( IDirectInputDevice8 *pdiDevice, uint32_t dwFormatSize )
{
	HRESULT hRes;
	DIDEVCAPS didCaps;
	DIDEVICEINSTANCE didInstance;

	memset( &didInstance, 0, sizeof( DIDEVICEINSTANCE ) );
	didInstance.dwSize = sizeof( DIDEVICEINSTANCE );
	hRes = pdiDevice->GetDeviceInfo( &didInstance );
	if ( FAILED( hRes ) )
		return;

	memset( &didCaps, 0, sizeof( DIDEVCAPS ) );
	didCaps.dwSize = sizeof( DIDEVCAPS );
	hRes = pdiDevice->GetCapabilities( &didCaps );
	if ( FAILED( hRes ) )
		return;
	
	SInputDevice siDevice;
	siDevice.nID = devices.size();
	siDevice.bPoll = ( didCaps.dwFlags & DIDC_POLLEDDATAFORMAT ) ? true : false;
	siDevice.szName = didInstance.tszProductName;
	siDevice.dwDevType = didInstance.dwDevType;
	siDevice.dwFormatSize = dwFormatSize;
	siDevice.pdiDevice = pdiDevice;

	AddDeviceKeys( siDevice.nID, GET_DIDEVICE_TYPE( didInstance.dwDevType ) );

	devices.push_back( siDevice );

	return;
}

// Добавить информацию про неизвестный девайс
static void AddDeviceEnum( IDirectInputDevice8 *pdiDevice )
{
	HRESULT hRes;
	DIDEVCAPS didCaps;
	DIDEVICEINSTANCE didInstance;
	
	memset( &didInstance, 0, sizeof( DIDEVICEINSTANCE ) );
	didInstance.dwSize = sizeof( DIDEVICEINSTANCE );
	hRes = pdiDevice->GetDeviceInfo( &didInstance );
	if ( FAILED( hRes ) )
		return;
	
	memset( &didCaps, 0, sizeof( DIDEVCAPS ) );
	didCaps.dwSize = sizeof( DIDEVCAPS );
	hRes = pdiDevice->GetCapabilities( &didCaps );
	if ( FAILED( hRes ) )
		return;
	
	SInputDevice siDevice;
	siDevice.nID = devices.size();
	siDevice.bPoll = false;
	siDevice.szName = didInstance.tszProductName;
	siDevice.dwDevType = didInstance.dwDevType;
	siDevice.dwFormatSize = sizeof(SInputDataFormat);
	siDevice.pdiDevice = pdiDevice;
	if ( ( didCaps.dwFlags & DIDC_POLLEDDATAFORMAT ) || ( didCaps.dwFlags & DIDC_POLLEDDEVICE  ) )
		siDevice.bPoll = true;

	DebugTrace("INPUT: polled device: %s\n", siDevice.bPoll ? "yes" : "no" );
	
	SInputDeviceEnum sDeviceEnum;
	sDeviceEnum.nID = siDevice.nID;
	sDeviceEnum.szName = MakeDeviceEnumName( GET_DIDEVICE_TYPE( didInstance.dwDevType ) );

	hRes = pdiDevice->EnumObjects( EnumDeviceObjectsCallback, &sDeviceEnum, DIDFT_ALL );
	if ( FAILED(hRes) )
		return;
	
	DIDATAFORMAT diDataFormat;
	diDataFormat.dwSize			= sizeof(DIDATAFORMAT);
	diDataFormat.dwObjSize	= sizeof(DIOBJECTDATAFORMAT);
	diDataFormat.dwDataSize	= siDevice.dwFormatSize;
	diDataFormat.dwFlags		= DIDF_ABSAXIS;
	diDataFormat.dwNumObjs	= sDeviceEnum.nNumControls;
	diDataFormat.rgodf			= &( sDeviceEnum.vectorObjects[0] );
	hRes = pdiDevice->SetDataFormat( &diDataFormat );
	if( FAILED(hRes) )
		return;

	DIPROPDWORD dipdw;
	dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj        = 0;
	dipdw.diph.dwHow        = DIPH_DEVICE;
	dipdw.dwData            = SAMPLE_BUFFER_SIZE;
	hRes = pdiDevice->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph );
	if( FAILED(hRes) )
		return;

	for ( std::vector<DIOBJECTDATAFORMAT>::iterator iTemp = sDeviceEnum.vectorObjects.begin(); iTemp != sDeviceEnum.vectorObjects.end(); iTemp++ )
	{
		if ( ( iTemp->dwType & DIDFT_AXIS ) == 0 )
			continue;

		DIPROPRANGE dipRange; 
		dipRange.diph.dwSize       = sizeof(DIPROPRANGE); 
		dipRange.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
		dipRange.diph.dwHow        = DIPH_BYID; 
		dipRange.diph.dwObj        = iTemp->dwType;
		dipRange.lMin              = -AXIS_RANGE_VALUE;
		dipRange.lMax              = AXIS_RANGE_VALUE;

		pdiDevice->SetProperty( DIPROP_RANGE, &dipRange.diph );
	}

	devices.push_back( siDevice );

	return;
}

#endif // BOOST_OS_WINDOWS

// Занести в hash действия для данного устройства
static void AddDeviceKey( int nDeviceID, int nDevType, int nDevAction, EControlType cType, const char *pszName )
{
	int nAction = INPUT_KEYID( nDeviceID, nDevAction );
	SKey &sActionKey = actionIDs[ nAction ];

	sActionKey.eType = cType;
	sActionKey.nAction = nAction;
	sActionKey.nDevType = nDevType;
	nameIDs[ pszName ] = nAction;
	idNames[ nAction ] = pszName;
}

// Имя устройства, под которым будут перечислены его контролы
//
// The counters are per kind, so a second gamepad is GAMEPAD1. A user's cfg binds
// these names, so the numbering is the order the devices were enumerated in and
// nothing else: plug a stick in after the config has been read and its binds do
// not attach.
static std::string MakeDeviceEnumName( int nDevType )
{
	switch( nDevType )
	{
	case DI8DEVTYPE_GAMEPAD:
		return fmt::format( "GAMEPAD{}", nCounter[0]++ );
	case DI8DEVTYPE_DRIVING:
		return fmt::format( "DRIVING{}", nCounter[1]++ );
	case DI8DEVTYPE_JOYSTICK:
		return fmt::format( "JOYSTICK{}", nCounter[2]++ );
	default:
		return fmt::format( "GAMECTRL{}", nCounter[3]++ );
	}
}

// Занести в hash один контрол игрового устройства
//
// A POV gets two more actions beside itself, the X and Y an axis binding reads;
// PumpMessages fills them from the cosine and sine of the reported angle.
static void AddDeviceControl( int nDeviceID, int nDevType, int nOfs, EControlType eType, const std::string &szName )
{
	SKey sKey;
	sKey.eType = eType;
	sKey.nAction = INPUT_KEYID( nDeviceID, nOfs );
	sKey.nDevType = nDevType;

	nameIDs[szName] = sKey.nAction;
	idNames[ sKey.nAction ] = szName;
	actionIDs[ sKey.nAction ] = sKey;

	DebugTrace("INPUT:\tNew control found! Add new control %s\n", szName.c_str() );

	if ( eType != CT_POV )
		return;

	sKey.nAction = INPUT_KEYIDEX( nDeviceID, nOfs, 1 );
	sKey.ePOVAxis = PA_X;
	nameIDs[szName + "_X"] = sKey.nAction;
	idNames[ sKey.nAction ] = szName + "_X";
	actionIDs[ sKey.nAction ] = sKey;

	DebugTrace("INPUT:\tNew control found! Add new control %s\n", ( szName + "_X" ).c_str() );

	sKey.nAction = INPUT_KEYIDEX( nDeviceID, nOfs, 2 );
	sKey.ePOVAxis = PA_Y;
	nameIDs[szName + "_Y"] = sKey.nAction;
	idNames[ sKey.nAction ] = szName + "_Y";
	actionIDs[ sKey.nAction ] = sKey;

	DebugTrace("INPUT:\tNew control found! Add new control %s\n", ( szName + "_Y" ).c_str() );
}

static void AddDeviceKeys( int nID, int nDevType )
{
	int nTemp = 0;
	while( kiKeyInfoList[nTemp].nDevType != 0 )
	{
		const SKeyInfo &key = kiKeyInfoList[ nTemp ];
		if ( key.nDevType == nDevType )
		{
			AddDeviceKey( nID, nDevType, key.nDevAction,  key.cType, key.pszName );
			// add dbl clk event
			if ( key.cType == CT_KEY )
			{
				std::string szName = std::string( key.pszName ) + "_DBLCLK";
				AddDeviceKey( nID, nDevType, key.nDevAction | DIK_DBLCLK_MODIFIER,  key.cType, szName.c_str() );
			}
		}
		nTemp++;
	}
}

#if BOOST_OS_WINDOWS

static BOOL CALLBACK EnumDevicesCallback( const DIDEVICEINSTANCE* pdidInstance, PVOID pContext )
{
	HRESULT hRes;
	NWin32Helper::com_ptr<IDirectInputDevice8> pdiTempDevice;
	
	hRes = pdiInput->CreateDevice( pdidInstance->guidInstance, pdiTempDevice.GetAddr(), NULL );
	if( FAILED( hRes ) ) 
		return DIENUM_CONTINUE;
		
	DebugTrace("INPUT: New device found! Add new device %s\n", pdidInstance->tszProductName );

	AddDeviceEnum( pdiTempDevice );

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumDeviceObjectsCallback( const DIDEVICEOBJECTINSTANCE* lpdidObject, PVOID pContext )
{
	SInputDeviceEnum *psDeviceEnum = static_cast<SInputDeviceEnum*>(pContext);
	ASSERT( psDeviceEnum != 0 );

	std::string szControlName;
	EControlType eType = CT_KEY;
	DIOBJECTDATAFORMAT diObjectFormat;

	if ( lpdidObject->guidType == GUID_XAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_X";
		diObjectFormat.pguid = &GUID_XAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lX );
	}
	else if ( lpdidObject->guidType == GUID_YAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_Y";
		diObjectFormat.pguid = &GUID_YAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lY );
	}
	else if ( lpdidObject->guidType == GUID_ZAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_Z";
		diObjectFormat.pguid = &GUID_ZAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lZ );
	}
	else if ( lpdidObject->guidType == GUID_RxAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_RX";
		diObjectFormat.pguid = &GUID_RxAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lRX );
	}
	else if ( lpdidObject->guidType == GUID_RyAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_RY";
		diObjectFormat.pguid = &GUID_RyAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lRY );
	}
	else if ( lpdidObject->guidType == GUID_RzAxis )
	{
		eType = CT_LIMAXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += "_AXIS_RZ";
		diObjectFormat.pguid = &GUID_RzAxis;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lRZ );
	}
	else if ( lpdidObject->guidType == GUID_POV )
	{
		eType = CT_POV;
		szControlName = psDeviceEnum->szName;
		szControlName += "_POV";
		diObjectFormat.pguid = &GUID_POV;
		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, lPOV );
	}
	else if ( lpdidObject->guidType == GUID_Slider )
	{
		eType = CT_AXIS;
		szControlName = psDeviceEnum->szName;
		szControlName += fmt::format( "_SLIDER{}", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Slider;

		return DIENUM_CONTINUE;
	}
	else if ( lpdidObject->guidType == GUID_Button )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += fmt::format( "_BUTTON{}", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Button;

		if( DIDFT_GETINSTANCE( lpdidObject->dwType ) > 32 )
			return DIENUM_CONTINUE;

		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, bButton[ DIDFT_GETINSTANCE( lpdidObject->dwType ) ] );
	}
	else if ( lpdidObject->guidType == GUID_Key )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += fmt::format( "_KEY{}", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Key;

		return DIENUM_CONTINUE;
	}
	else if ( lpdidObject->guidType == GUID_Unknown )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += fmt::format( "_UNKNOWN{}", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Unknown;

		return DIENUM_CONTINUE;
	}

	diObjectFormat.dwType = lpdidObject->dwType;
	diObjectFormat.dwFlags = 0;
	psDeviceEnum->nNumControls++;
	psDeviceEnum->vectorObjects.push_back( diObjectFormat );

	AddDeviceControl( psDeviceEnum->nID, GET_DIDEVICE_TYPE( diObjectFormat.dwType ), diObjectFormat.dwOfs, eType, szControlName );

	return DIENUM_CONTINUE;
}

#endif // BOOST_OS_WINDOWS

#if !BOOST_OS_WINDOWS

//
// Устройства ввода поверх SDL
//
// DirectInput hands out a per-device ring buffer that the driver fills behind
// the application's back and GetDeviceData drains on demand. SDL's event queue
// is the same thing: the backend appends every transition with a timestamp of
// its own and nothing is lost between two drains. What it is not is a second
// queue - there is one, and System/WinFrame is already polling it for the
// window and for the logical keys the UI reads.
//
// So this does not poll. It registers an event watch, which SDL calls as each
// event is pushed, and copies the device events it cares about into a buffer of
// its own. WinFrame goes on polling and sees everything it saw before, and
// PumpMessages below drains this buffer on exactly the schedule GetDeviceData
// was drained on. The watch may run on whichever thread pumped the event, hence
// the mutex; watchMutex covers watchedEvents, dwWatchSequence, nMouseState and
// bWatchOverflow, and nothing else.
//
// Immediate state - SDL_GetKeyboardState and friends - is deliberately not the
// path here. Every SMessage this module emits is an edge rather than a level,
// the double click synthesis in PumpMessages needs the time a key went down,
// and the accumulators in Input/BindInternal.h integrate power over the gap
// between two event timestamps. Polled state carries none of that and would
// drop any press shorter than a frame. It is used only by ReadDeviceState,
// which asks what is held down right now, which is what it wants to know.

namespace
{

//! One buffered event, tagged with the device that produced it.
struct SWatchedEvent
{
	int nDeviceID;
	DIDEVICEOBJECTDATA did;
};

std::mutex watchMutex;
std::vector<SWatchedEvent> watchedEvents;
uint32_t dwWatchSequence = 0;
bool bWatchOverflow = false;

// The mouse axes as DIPROPAXISMODE_ABS reported them: running totals rather than
// deltas, because FillEventInfo takes the difference between an event's value
// and the last one it saw. The wheel is one of them, which is why it has to be
// accumulated as well rather than passed straight through.
int32_t nMouseState[3] = { 0, 0, 0 };

int nKeyboardDeviceID = -1;
int nMouseDeviceID = -1;

//! An SDL event timestamp on the clock GetCurrentTimeMilliseconds returns.
//!
//! SDL counts nanoseconds from its own initialisation and the engine counts
//! milliseconds from an unspecified steady_clock epoch, so the two cannot be
//! compared. Converting through the event's age rather than through the epochs
//! keeps the spacing between two events in a frame, which is what the
//! accumulators integrate over.
uint32_t EventTimeMilliseconds( uint64_t nTimestampNs )
{
	const uint64_t nNowNs = SDL_GetTicksNS();
	const uint64_t nAgeNs = nNowNs > nTimestampNs ? nNowNs - nTimestampNs : 0;
	return GetCurrentTimeMilliseconds() - (uint32_t)( nAgeNs / 1000000 );
}

//! A stick axis on the range AddDeviceEnum asks DirectInput for.
//!
//! Not cosmetic. POWER_MIN_LIMIT in Input/BindInternal.h is an absolute
//! threshold on the accumulated power, and every bindconfigure coefficient a cfg
//! sets is calibrated against this range, so reporting SDL's own would move both
//! by a factor of three.
int32_t ScaleJoystickAxis( int16_t nValue )
{
	return ( (int32_t)nValue * AXIS_RANGE_VALUE ) / 32767;
}

//! A hat position as a POV angle in hundredths of a degree, clockwise from up,
//! which is how DirectInput reports one. 0xFFFFFFFF is centred.
uint32_t HatToPov( uint8_t nHat )
{
	switch ( nHat )
	{
	case SDL_HAT_UP: return 0;
	case SDL_HAT_RIGHTUP: return 4500;
	case SDL_HAT_RIGHT: return 9000;
	case SDL_HAT_RIGHTDOWN: return 13500;
	case SDL_HAT_DOWN: return 18000;
	case SDL_HAT_LEFTDOWN: return 22500;
	case SDL_HAT_LEFT: return 27000;
	case SDL_HAT_LEFTUP: return 31500;
	default: return 0xFFFFFFFF;
	}
}

//! The control offset of a stick axis within SInputDataFormat, or -1 for an axis
//! past the six that format names. DirectInput gave those to sliders, which
//! EnumDeviceObjectsCallback declines to register, so they are dropped here too.
int JoystickAxisOffset( int nAxis )
{
	switch ( nAxis )
	{
	case 0: return (int)offsetof( SInputDataFormat, lX );
	case 1: return (int)offsetof( SInputDataFormat, lY );
	case 2: return (int)offsetof( SInputDataFormat, lZ );
	case 3: return (int)offsetof( SInputDataFormat, lRX );
	case 4: return (int)offsetof( SInputDataFormat, lRY );
	case 5: return (int)offsetof( SInputDataFormat, lRZ );
	default: return -1;
	}
}

//! The device type DirectInput would have reported, which decides the name
//! prefix a cfg binds: GAMEPAD0, DRIVING0, JOYSTICK0 or GAMECTRL0.
int JoystickDeviceType( SDL_JoystickType eType )
{
	switch ( eType )
	{
	case SDL_JOYSTICK_TYPE_GAMEPAD: return DI8DEVTYPE_GAMEPAD;
	case SDL_JOYSTICK_TYPE_WHEEL: return DI8DEVTYPE_DRIVING;
	case SDL_JOYSTICK_TYPE_ARCADE_STICK:
	case SDL_JOYSTICK_TYPE_FLIGHT_STICK: return DI8DEVTYPE_JOYSTICK;
	default: return DI8DEVTYPE_DEVICE;
	}
}

//! The device an SDL joystick event came from, or null once it has been removed.
SInputDevice *FindJoystick( SDL_JoystickID id )
{
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( iTempDevice->pJoystick != 0 && iTempDevice->joystickID == id )
		{
			return &( *iTempDevice );
		}
	}
	return 0;
}

//! Buffer one transition. Called with watchMutex held.
void PushEvent( int nDeviceID, uint32_t dwOfs, uint32_t dwData, uint64_t nTimestampNs )
{
	if ( nDeviceID < 0 )
	{
		return;
	}
	// The same bound DIPROP_BUFFERSIZE puts on a DirectInput device, and the
	// same consequence when it is reached: the transition is lost and every
	// device is resynced from its current state, rather than the buffer growing
	// without limit. ReadDeviceData below is what acts on the flag.
	if ( watchedEvents.size() >= (size_t)SAMPLE_BUFFER_SIZE * devices.size() )
	{
		bWatchOverflow = true;
		return;
	}

	SWatchedEvent sEvent;
	sEvent.nDeviceID = nDeviceID;
	sEvent.did.dwOfs = dwOfs;
	sEvent.did.dwData = dwData;
	sEvent.did.dwTimeStamp = EventTimeMilliseconds( nTimestampNs );
	// One counter across every device, because PumpMessages sorts the events of
	// all of them together and a per-device counter would interleave wrongly.
	sEvent.did.dwSequence = dwWatchSequence++;
	watchedEvents.push_back( sEvent );
}

bool SDLCALL EventWatch( void *pUserData, SDL_Event *pEvent )
{
	std::lock_guard<std::mutex> lock( watchMutex );

	switch ( pEvent->type )
	{
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
	{
		// Auto repeat is the window system's, not the keyboard's. A DirectInput
		// device reports one transition down and one up however long a key is
		// held, and the bindings are written against that: a repeat here would
		// re-fire every event bind for as long as the key was down.
		if ( pEvent->key.repeat )
		{
			break;
		}
		const int nKey = SdlScancodeToDirectInputKey( pEvent->key.scancode );
		if ( nKey != 0 )
		{
			PushEvent( nKeyboardDeviceID, nKey, pEvent->key.down ? 0x80 : 0, pEvent->key.timestamp );
		}
		break;
	}

	case SDL_EVENT_MOUSE_MOTION:
		if ( pEvent->motion.xrel != 0.0f )
		{
			nMouseState[0] += (int32_t)pEvent->motion.xrel;
			PushEvent( nMouseDeviceID, DIMOFS_X, (uint32_t)nMouseState[0], pEvent->motion.timestamp );
		}
		if ( pEvent->motion.yrel != 0.0f )
		{
			nMouseState[1] += (int32_t)pEvent->motion.yrel;
			PushEvent( nMouseDeviceID, DIMOFS_Y, (uint32_t)nMouseState[1], pEvent->motion.timestamp );
		}
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		// The button's own count is ignored: the double click a bind can name is
		// synthesized in PumpMessages from TIME_DIFF_DBL_CLK, so that a cfg
		// written against the Windows build means the same thing here.
		const int nOfs = SdlMouseButtonToOffset( pEvent->button.button );
		if ( nOfs >= 0 )
		{
			// The event type is the edge SDL delivered. Deriving the state from it
			// keeps a DOWN and an UP from ever taking the same path even if a backend
			// leaves the redundant payload field in an unexpected state.
			const bool bPressed = pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
			PushEvent( nMouseDeviceID, nOfs, bPressed ? 0x80 : 0, pEvent->button.timestamp );
		}
		break;
	}

	case SDL_EVENT_MOUSE_WHEEL:
	{
		// SDL counts notches and DirectInput counts a notch as WHEEL_DELTA, which
		// is what ReadControlGranularity reports and what every bindconfigure of
		// MOUSE_AXIS_Z is scaled against.
		float fDelta = pEvent->wheel.y;
		if ( pEvent->wheel.direction == SDL_MOUSEWHEEL_FLIPPED )
		{
			fDelta = -fDelta;
		}
		if ( fDelta != 0.0f )
		{
			nMouseState[2] += (int32_t)( fDelta * MOUSE_WHEEL_GRANULARITY );
			PushEvent( nMouseDeviceID, DIMOFS_Z, (uint32_t)nMouseState[2], pEvent->wheel.timestamp );
		}
		break;
	}

	case SDL_EVENT_JOYSTICK_AXIS_MOTION:
	{
		const SInputDevice *pDevice = FindJoystick( pEvent->jaxis.which );
		const int nOfs = JoystickAxisOffset( pEvent->jaxis.axis );
		if ( pDevice != 0 && nOfs >= 0 )
		{
			PushEvent( pDevice->nID, nOfs, (uint32_t)ScaleJoystickAxis( pEvent->jaxis.value ), pEvent->jaxis.timestamp );
		}
		break;
	}

	case SDL_EVENT_JOYSTICK_HAT_MOTION:
	{
		// SInputDataFormat has room for one POV, so a second hat is not reported.
		const SInputDevice *pDevice = FindJoystick( pEvent->jhat.which );
		if ( pDevice != 0 && pEvent->jhat.hat == 0 )
		{
			PushEvent( pDevice->nID, (uint32_t)offsetof( SInputDataFormat, lPOV ), HatToPov( pEvent->jhat.value ),
			           pEvent->jhat.timestamp );
		}
		break;
	}

	case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
	case SDL_EVENT_JOYSTICK_BUTTON_UP:
	{
		const SInputDevice *pDevice = FindJoystick( pEvent->jbutton.which );
		if ( pDevice != 0 && pEvent->jbutton.button < JOYSTICK_BUTTON_COUNT )
		{
			PushEvent( pDevice->nID, (uint32_t)( offsetof( SInputDataFormat, bButton ) + pEvent->jbutton.button ),
			           pEvent->jbutton.down ? 0x80 : 0, pEvent->jbutton.timestamp );
		}
		break;
	}

	default:
		break;
	}

	// The watch is an observer. Returning true leaves the event on the queue for
	// System/WinFrame to poll, which is where the window and the UI's keys come
	// from; filtering it out here would take those with it.
	return true;
}

//! Register the keyboard and the mouse, which SDL reports without a handle.
void AddSystemDevice( int nDevType, uint32_t dwFormatSize, const char *pszName, int *pnDeviceID )
{
	SInputDevice siDevice;
	siDevice.nID = devices.size();
	siDevice.szName = pszName;
	siDevice.dwDevType = nDevType;
	siDevice.dwFormatSize = dwFormatSize;

	AddDeviceKeys( siDevice.nID, nDevType );

	*pnDeviceID = siDevice.nID;
	devices.push_back( siDevice );
}

//! Register one stick, and synthesize a control name for each of its axes, hats
//! and buttons the way EnumDeviceObjectsCallback does on Windows.
void AddJoystickDevice( SDL_Joystick *pJoystick )
{
	SInputDevice siDevice;
	siDevice.nID = devices.size();
	const char *pszJoystickName = SDL_GetJoystickName( pJoystick );
	siDevice.szName = pszJoystickName != 0 ? pszJoystickName : "";
	siDevice.dwDevType = JoystickDeviceType( SDL_GetJoystickType( pJoystick ) );
	siDevice.dwFormatSize = sizeof( SInputDataFormat );
	siDevice.pJoystick = pJoystick;
	siDevice.joystickID = SDL_GetJoystickID( pJoystick );

	const int nDevType = GET_DIDEVICE_TYPE( siDevice.dwDevType );
	const std::string szDeviceName = MakeDeviceEnumName( nDevType );

	DebugTrace( "INPUT: New device found! Add new device %s as %s\n", siDevice.szName.c_str(), szDeviceName.c_str() );

	// The axis suffixes are DirectInput's object names, in its own order, because
	// that is what a cfg binds. SDL numbers a stick's axes rather than naming
	// them, and on every device that reports six the numbering is the same one.
	static const char *pszAxisSuffix[] = { "_AXIS_X", "_AXIS_Y", "_AXIS_Z", "_AXIS_RX", "_AXIS_RY", "_AXIS_RZ" };
	const int nAxes = SDL_GetNumJoystickAxes( pJoystick );
	for ( int nAxis = 0; nAxis < nAxes; ++nAxis )
	{
		const int nOfs = JoystickAxisOffset( nAxis );
		if ( nOfs < 0 )
		{
			continue;
		}
		AddDeviceControl( siDevice.nID, nDevType, nOfs, CT_LIMAXIS, szDeviceName + pszAxisSuffix[nAxis] );
	}

	if ( SDL_GetNumJoystickHats( pJoystick ) > 0 )
	{
		AddDeviceControl( siDevice.nID, nDevType, (int)offsetof( SInputDataFormat, lPOV ), CT_POV,
		                  szDeviceName + "_POV" );
	}

	const int nButtons = SDL_GetNumJoystickButtons( pJoystick );
	for ( int nButton = 0; nButton < nButtons && nButton < JOYSTICK_BUTTON_COUNT; ++nButton )
	{
		AddDeviceControl( siDevice.nID, nDevType, (int)( offsetof( SInputDataFormat, bButton ) + nButton ), CT_KEY,
		                  szDeviceName + fmt::format( "_BUTTON{}", nButton ) );
	}

	devices.push_back( siDevice );
}

}

static bool OpenDevices( int nSampleBufferSize )
{
	// nSampleBufferSize sets DIPROP_BUFFERSIZE on Windows. Here the buffer is
	// SDL's own event queue, which grows as it needs to, and the cap PushEvent
	// applies is derived from SAMPLE_BUFFER_SIZE like the keyboard's is there.
	(void)nSampleBufferSize;

	if ( !NSdl::EnsureVideo() )
	{
		return false;
	}

	AddSystemDevice( DI8DEVTYPE_MOUSE, DIMOUSESTATE2_SIZE, "System Mouse", &nMouseDeviceID );
	AddSystemDevice( DI8DEVTYPE_KEYBOARD, DIKEYBOARDSTATE_SIZE, "System Keyboard", &nKeyboardDeviceID );

	// A machine with no stick attached is the normal case, so a joystick
	// subsystem that will not start is not a reason to fail: the game is
	// playable on the keyboard and the mouse, which are already registered.
	//
	// Enumerated once, as DirectInput's EnumDevices was. A stick plugged in
	// later is not picked up, because the names here are positional - GAMEPAD0
	// is whichever gamepad was first - and a cfg that has already been read has
	// resolved those names to action ids that cannot be handed to a new device.
	if ( NSdl::EnsureJoystick() )
	{
		int nCount = 0;
		SDL_JoystickID *pIDs = SDL_GetJoysticks( &nCount );
		if ( pIDs != 0 )
		{
			for ( int i = 0; i < nCount; ++i )
			{
				SDL_Joystick *pJoystick = SDL_OpenJoystick( pIDs[i] );
				if ( pJoystick != 0 )
				{
					AddJoystickDevice( pJoystick );
				}
			}
			SDL_free( pIDs );
		}
		SDL_SetJoystickEventsEnabled( true );
	}

	if ( !SDL_AddEventWatch( EventWatch, 0 ) )
	{
		csSystem << CC_RED << "Cannot watch SDL input events: " << SDL_GetError() << endl;
		return false;
	}

	return true;
}

static void CloseDevices()
{
	SDL_RemoveEventWatch( EventWatch, 0 );

	{
		std::lock_guard<std::mutex> lock( watchMutex );
		watchedEvents.clear();
		bWatchOverflow = false;
	}

	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( iTempDevice->pJoystick != 0 )
		{
			SDL_CloseJoystick( iTempDevice->pJoystick );
			iTempDevice->pJoystick = 0;
		}
	}

	nKeyboardDeviceID = -1;
	nMouseDeviceID = -1;
}

static bool SetCoopLevel()
{
	// SDL has no cooperative level to set. Nor is one needed: SetCoopLevel on
	// Windows asks for DISCL_EXCLUSIVE only for a game controller, and always
	// DISCL_NONEXCLUSIVE for the keyboard and the mouse.
	bCoopLevelSet = true;
	return true;
}

static bool AcquireDevices( bool bAcquire )
{
	// There is nothing to acquire: SDL stops delivering keyboard and mouse events
	// on its own when the window is no longer the one being typed into. What
	// Unacquire also did was discard whatever the device had buffered, so that
	// nothing from before the switch arrived after it, and that part is real.
	if ( !bAcquire )
	{
		std::lock_guard<std::mutex> lock( watchMutex );
		watchedEvents.clear();
	}
	return true;
}

static bool IsWindowFocused()
{
	if ( hWindow == 0 )
	{
		return false;
	}
	return ( SDL_GetWindowFlags( AsSdlWindow( hWindow ) ) & SDL_WINDOW_INPUT_FOCUS ) != 0;
}

static void PumpDeviceEvents()
{
	// What moves transitions out of the window system and onto SDL's queue, which
	// is what runs the watch above. Called here rather than relying on WinFrame's
	// poll so that this drain keeps its own schedule, which is what PurgeEvents
	// and PurgeUIEvents expect when they pump and read in one go.
	SDL_PumpEvents();
}

static bool ReadDeviceData( SInputDevice &sDevice, DIDEVICEOBJECTDATA *pObjects, unsigned long *pdwElements )
{
	std::lock_guard<std::mutex> lock( watchMutex );

	if ( bWatchOverflow )
	{
		// Whatever was dropped could have belonged to any of them, so they all
		// have to be read back from their current state.
		bWatchOverflow = false;
		for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
		{
			iTempDevice->bNeedResync = true;
		}
	}

	const unsigned long dwMax = *pdwElements;
	unsigned long dwCount = 0;
	std::vector<SWatchedEvent> rest;
	rest.reserve( watchedEvents.size() );
	for ( std::vector<SWatchedEvent>::const_iterator iTempEvent = watchedEvents.begin(); iTempEvent != watchedEvents.end();
	      ++iTempEvent )
	{
		if ( iTempEvent->nDeviceID == sDevice.nID && dwCount < dwMax )
		{
			pObjects[dwCount++] = iTempEvent->did;
		}
		else
		{
			rest.push_back( *iTempEvent );
		}
	}
	watchedEvents.swap( rest );

	*pdwElements = dwCount;
	return true;
}

static void ReadDeviceState( const SInputDevice &sDevice, std::vector<uint8_t> *pBuffer )
{
	uint8_t *pData = &( ( *pBuffer )[0] );
	memset( pData, 0, pBuffer->size() );

	switch ( GET_DIDEVICE_TYPE( sDevice.dwDevType ) )
	{
	case DI8DEVTYPE_KEYBOARD:
	{
		// Read live rather than from anything this file kept, because the point
		// of a resync is to find out what changed while events were not arriving.
		int nKeys = 0;
		const bool *pKeys = SDL_GetKeyboardState( &nKeys );
		if ( pKeys == 0 )
		{
			break;
		}
		for ( int i = 0; i < DIRECT_INPUT_KEY_COUNT; ++i )
		{
			const SDL_Scancode scancode = directInputKeys[i].scancode;
			if ( (int)scancode < nKeys && pKeys[scancode] )
			{
				pData[directInputKeys[i].nKey] = 0x80;
			}
		}
		break;
	}

	case DI8DEVTYPE_MOUSE:
	{
		// The axes come from the running totals rather than from the pointer's
		// position, so that regaining focus does not read as one enormous move.
		{
			std::lock_guard<std::mutex> lock( watchMutex );
			memcpy( pData + DIMOFS_X, &nMouseState[0], sizeof( int32_t ) );
			memcpy( pData + DIMOFS_Y, &nMouseState[1], sizeof( int32_t ) );
			memcpy( pData + DIMOFS_Z, &nMouseState[2], sizeof( int32_t ) );
		}
		// SDL_GetMouseState answers for the five buttons it has a mask for, which
		// is where SdlMouseButtonToOffset stops being able to ask.
		const SDL_MouseButtonFlags dwButtons = SDL_GetMouseState( 0, 0 );
		for ( int nButton = SDL_BUTTON_LEFT; nButton <= SDL_BUTTON_X2; ++nButton )
		{
			const int nOfs = SdlMouseButtonToOffset( nButton );
			if ( nOfs >= 0 && ( dwButtons & SDL_BUTTON_MASK( nButton ) ) != 0 )
			{
				pData[nOfs] = 0x80;
			}
		}
		break;
	}

	default:
	{
		if ( sDevice.pJoystick == 0 )
		{
			break;
		}
		const int nAxes = SDL_GetNumJoystickAxes( sDevice.pJoystick );
		for ( int nAxis = 0; nAxis < nAxes; ++nAxis )
		{
			const int nOfs = JoystickAxisOffset( nAxis );
			if ( nOfs < 0 )
			{
				continue;
			}
			const int32_t nValue = ScaleJoystickAxis( SDL_GetJoystickAxis( sDevice.pJoystick, nAxis ) );
			memcpy( pData + nOfs, &nValue, sizeof( nValue ) );
		}
		if ( SDL_GetNumJoystickHats( sDevice.pJoystick ) > 0 )
		{
			const uint32_t dwPov = HatToPov( SDL_GetJoystickHat( sDevice.pJoystick, 0 ) );
			memcpy( pData + offsetof( SInputDataFormat, lPOV ), &dwPov, sizeof( dwPov ) );
		}
		const int nButtons = SDL_GetNumJoystickButtons( sDevice.pJoystick );
		for ( int nButton = 0; nButton < nButtons && nButton < JOYSTICK_BUTTON_COUNT; ++nButton )
		{
			if ( SDL_GetJoystickButton( sDevice.pJoystick, nButton ) )
			{
				pData[offsetof( SInputDataFormat, bButton ) + nButton] = 0x80;
			}
		}
		break;
	}
	}
}

static float ReadControlGranularity( const SKey &sKey )
{
	// The wheel is the only control that does not move one unit at a time, and
	// DIPROP_GRANULARITY is how the Windows path learns the same thing.
	if ( sKey.nDevType == DI8DEVTYPE_MOUSE && INPUT_GETACTIONOFFS( sKey.nAction ) == DIMOFS_Z )
	{
		return MOUSE_WHEEL_GRANULARITY;
	}
	return 1.0f;
}

static std::string ReadControlLocalName( int nAction )
{
	const SKey &sKey = actionIDs[nAction];

	// For a key this is better than what GetObjectInfo answers on Windows: SDL
	// names the key by what the current layout prints on it, so a tutorial line
	// naming a key reads correctly on a keyboard that is not a US one.
	if ( sKey.nDevType == DI8DEVTYPE_KEYBOARD )
	{
		const SDL_Scancode scancode = DirectInputKeyToSdlScancode( INPUT_GETACTIONOFFS( sKey.nAction ) & ~DIK_DBLCLK_MODIFIER );
		if ( scancode == SDL_SCANCODE_UNKNOWN )
		{
			return "";
		}
		const char *pszName = SDL_GetKeyName( SDL_GetKeyFromScancode( scancode, SDL_KMOD_NONE, false ) );
		return pszName != 0 ? pszName : "";
	}

	// SDL names a stick but not the individual axes and buttons on it, so the
	// name this module synthesized is the most specific thing there is to give.
	std::unordered_map<int, std::string>::const_iterator iTempName = idNames.find( nAction );
	return iTempName != idNames.end() ? iTempName->second : "";
}

#endif // !BOOST_OS_WINDOWS


bool ConvertMessage( const NWinFrame::SWindowsMsg &rWindowMsg, std::string *pszGameMessage, int *pnParam1, int *pnParam2, int *pnCount, NInput::EControlType *peControlType )
{
	NI_ASSERT( pszGameMessage != 0, "Wrong Parameter: pszGameMessage == 0" );
	NI_ASSERT( pnParam1 != 0, "Wrong Parameter: pnParam1 == 0" );
	NI_ASSERT( pnParam2 != 0, "Wrong Parameter: pnParam2 == 0" );
	NI_ASSERT( pnCount != 0, "Wrong Parameter: pnCount == 0" );
	NI_ASSERT( peControlType != 0, "Wrong Parameter: peControlType == 0" );
	//
	if ( rWindowMsg.msg == NWinFrame::SWindowsMsg::CHAR )
	{
		const std::string szCharBuffer = std::string() + (char)( rWindowMsg.nKey );
		std::wstring wszCharBuffer;
		NStr::ToUnicode( &wszCharBuffer, szCharBuffer );
		if ( wszCharBuffer.size() == 1 )
		{
			( *pszGameMessage ) = "win_char";
			( *pnParam1 ) = wszCharBuffer[0];
			( *pnParam2 ) = 0;
			( *pnCount ) = rWindowMsg.nRep;
		}
		return true;
	}
	if ( rWindowMsg.msg == NWinFrame::SWindowsMsg::KEY_DOWN )
	{
		( *pszGameMessage ) = "win_key";
		( *pnParam1 ) = rWindowMsg.nKey;
		( *pnParam2 ) = 0;
		( *pnCount ) = rWindowMsg.nRep;
		return true;
	}
	switch ( rWindowMsg.msg )
	{
	case NWinFrame::SWindowsMsg::MOUSE_MOVE: 
		( *pszGameMessage ) = "win_mouse_move";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::RB_DBLCLK: 
		( *pszGameMessage ) = "win_right_button_dblclk";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::LB_DBLCLK: 
		( *pszGameMessage ) = "win_left_button_dblclk";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::RB_DOWN: 
		( *pszGameMessage ) = "win_right_button_down";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::LB_DOWN: 
		( *pszGameMessage ) = "win_left_button_down";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::RB_UP: 
		( *pszGameMessage ) = "win_right_button_up";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::LB_UP: 
		( *pszGameMessage ) = "win_left_button_up";
		*peControlType = NInput::CT_WINDOWS;
		break;
	case NWinFrame::SWindowsMsg::CLOSE: 
		( *pszGameMessage ) = "try_exit_windows";
		*peControlType = NInput::CT_UNKNOWN;
		break;
	default:
		return false;
	}
	( *pnParam1 ) = PackCoords( CVec2( rWindowMsg.x, rWindowMsg.y ) );
	( *pnParam2 ) = rWindowMsg.dwFlags;
	( *pnCount ) = 1;
	return true;
}

}; // end of namespace NInput

