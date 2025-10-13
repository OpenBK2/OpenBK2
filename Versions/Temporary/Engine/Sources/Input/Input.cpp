#include "stdafx.h"

#include <dinput.h>
#include "Input/Input.h"
#include "Input/GameMessage.h"
#include "Misc/Win32Helper.h"
#include "Misc/StrProc.h"

#include <cstdint>

extern "C" WINBASEAPI BOOL WINAPI IsDebuggerPresent(void);

bool bMouseDisabledDebug = false;
static const int POV_RANGE_VALUE = 1000;
static const int AXIS_RANGE_VALUE = 10000;
static const int SAMPLE_BUFFER_SIZE = 1024;
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
	NWin32Helper::com_ptr<IDirectInputDevice8> pdiDevice;
	//
	SInputDevice(): bPoll( false ), bNeedResync( false ), dwDevType( 0 ), pdiDevice( 0 ) {  }
};
struct SInputDeviceEnum
{
	int nID;
	int nNumControls;
	std::string szName;
	std::vector<DIOBJECTDATAFORMAT> vectorObjects;

	SInputDeviceEnum(): nID( 0 ), nNumControls( 0 ), szName( "" ) {}
};
struct SInputDataFormat
{
	LONG  lX;
	LONG  lY;
	LONG  lZ;
	LONG  lRX;
	LONG  lRY;
	LONG  lRZ;
	LONG  lPOV;
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
static NWin32Helper::com_ptr<IDirectInput8> pdiInput;
///
static STime sLastEventTime = 0;
static CEventList events;
static std::list<SMessage> messages;
///
static bool SetCoopLevel();
static bool SetFocus( bool bFocus );
static void ReleaseKeyboardState();
static void ResyncDevice( const SInputDevice &sDevice );
static void AddDeviceInfo( IDirectInputDevice8 *pdiDevice, uint32_t dwFormatSize );
static void AddDeviceEnum( IDirectInputDevice8 *pdiDevice );
static void AddDeviceKeys( int nID, int nDevType );
static BOOL CALLBACK EnumDevicesCallback( const DIDEVICEINSTANCE* pdidInstance, PVOID pContext );
static BOOL CALLBACK EnumDeviceObjectsCallback( const DIDEVICEOBJECTINSTANCE* lpdidObject, PVOID pContext );

//
// Initialization / Deinitialization / message handling
//

// Инициализировать DirectInput
bool InitInput( HWND hWnd, bool bDebugMouse, bool _bNonExclusiveMode, int nSampleBufferSize )
{
	HRESULT hRes;
	NWin32Helper::com_ptr<IDirectInputDevice8> pdiTempDevice;
	
	if ( bInitialized )
		return true;

	hWindow = hWnd;
	bNonExclusiveMode = _bNonExclusiveMode;

	hRes = DirectInput8Create( GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)pdiInput.GetAddr(), 0 );
	if( FAILED(hRes) )
		return false;

	// disable mouse in debugger for win2k and earlier versions
	bool bMouseEnabled = true;
	if ( IsDebuggerPresent() && !bDebugMouse )
	{
		// Figure out which OS we are on.
		OSVERSIONINFO stOSVI;
		memset( &stOSVI , NULL , sizeof(OSVERSIONINFO) );
		stOSVI.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
		if ( !GetVersionEx( &stOSVI ) )
			bMouseEnabled = false;
		else
		{
			if ( stOSVI.dwMajorVersion < 5 || ( stOSVI.dwMajorVersion == 5 && stOSVI.dwMinorVersion == 0 ) )
				bMouseEnabled = false;
		}
	}
	
	bMouseDisabledDebug = !bMouseEnabled;

	if ( bMouseEnabled )
	{
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
	devices.clear();
	pdiInput = 0;
	bInitialized = false;

	return true;
}

static bool SetCoopLevel()
{
	if ( bCoopLevelSet )
		return true;
	
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		HRESULT hRes;

		if ( ( IsDebuggerPresent() && ( GET_DIDEVICE_TYPE( iTempDevice->dwDevType ) != DI8DEVTYPE_MOUSE ) ) || bNonExclusiveMode )
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
	HRESULT hRes;
	
	if ( !bInitialized )
		return;

	// Background simulation may keep bFocus true after the window is deactivated.
	// DirectInput focus must still follow the actual foreground window.
	SetFocus( bFocus && GetForegroundWindow() == hWindow );
	if ( !bFocusCaptured )
		return;

	uint32_t dwTest = GetTickCount();
	if ( dwTest - dwPrevPump < 1 )
		return;
	dwPrevPump = dwTest;

	int nNumEvents = 0;
	events.resize( SAMPLE_BUFFER_SIZE * devices.size() * 2 );
	for ( CDevicesList::iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		unsigned long dwElements = SAMPLE_BUFFER_SIZE;
		DIDEVICEOBJECTDATA didObjects[SAMPLE_BUFFER_SIZE];

		if ( iTempDevice->bPoll )
		{
			hRes = iTempDevice->pdiDevice->Poll();
			if ( FAILED( hRes ) )
				iTempDevice->pdiDevice->Acquire();
		}

		hRes = iTempDevice->pdiDevice->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), didObjects, &dwElements, 0 );
		if ( hRes == DI_BUFFEROVERFLOW )
			iTempDevice->bNeedResync = true;
    if ( SUCCEEDED( hRes ) ) 
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
		else
			iTempDevice->pdiDevice->Acquire();
	}
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

	sDevice.pdiDevice->GetDeviceState( sDevice.dwFormatSize, &( sBuffer[0] ) );

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
					GetTickCount() )
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
		pMsg->tTime = GetTickCount();
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
	DIPROPDWORD diProp;

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
			*pfGranularity = 1.0f;
			break;
	}

	for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
	{
		if ( GET_DIDEVICE_TYPE( iTempDevice->dwDevType ) != sKey.nDevType )
			continue;

		diProp.diph.dwSize = sizeof( DIPROPDWORD );
		diProp.diph.dwHeaderSize = sizeof( DIPROPHEADER );
		diProp.diph.dwHow = DIPH_BYOFFSET;
		diProp.diph.dwObj = INPUT_GETACTIONOFFS( nAction );
		HRESULT hRes = iTempDevice->pdiDevice->GetProperty( DIPROP_GRANULARITY, (DIPROPHEADER*)&diProp );
		if ( SUCCEEDED(hRes) )
		{
			*pfGranularity = (float)diProp.dwData;
			break;
		}
	}

	return;
}

std::string GetControlLocalName( int nAction )
{
	if ( actionIDs.find( nAction ) == actionIDs.end() )
		return "";
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

void StartSaveInput( CDataStream *pStream )
{
}

void StopSaveInput()
{
}

void StartEmulateInput( CDataStream *pStream )
{
}

void StopEmulateInput()
{
}

//
//	Internal functions
//

// получит / отдать контроль над девайсами
static bool SetFocus( bool bFocus )
{
	HRESULT hRes;

	if ( !bInitialized )
		return false;
	if ( bFocusCaptured == bFocus )
		return true;
	if ( bFocus )
	{
		if ( !SetCoopLevel() )
			return false;

		for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
		{
			hRes = iTempDevice->pdiDevice->Acquire();
			if( FAILED( hRes ) )
				return false;

			ResyncDevice( *iTempDevice );
		}
	}
	else
	{
		// Key-up events are not delivered after DirectInput loses foreground access.
		// Release every tracked keyboard key so Alt+Tab cannot leave a bind active.
		ReleaseKeyboardState();

		for ( CDevicesList::const_iterator iTempDevice = devices.begin(); iTempDevice != devices.end(); ++iTempDevice )
		{
			hRes = iTempDevice->pdiDevice->Unacquire();
			if ( FAILED( hRes ) )
				return false;
		}
	}

	bFocusCaptured = bFocus;

	return true;
}

static void ReleaseKeyboardState()
{
	DebugTrace("Release keyboard state");
	const uint32_t dwTime = GetTickCount();
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

// добавить информацию про девайс
static void AddDeviceInfo( IDirectInputDevice8 *pdiDevice, uint32_t dwFormatSize )
{
	HRESULT hRes;
	DIDEVCAPS didCaps;
	DIDEVICEINSTANCE didInstance;

	ZeroMemory( &didInstance, sizeof( DIDEVICEINSTANCE ) );
	didInstance.dwSize = sizeof( DIDEVICEINSTANCE );
	hRes = pdiDevice->GetDeviceInfo( &didInstance );
	if ( FAILED( hRes ) )
		return;

	ZeroMemory( &didCaps, sizeof( DIDEVCAPS ) );
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
	
	ZeroMemory( &didInstance, sizeof( DIDEVICEINSTANCE ) );
	didInstance.dwSize = sizeof( DIDEVICEINSTANCE );
	hRes = pdiDevice->GetDeviceInfo( &didInstance );
	if ( FAILED( hRes ) )
		return;
	
	ZeroMemory( &didCaps, sizeof( DIDEVCAPS ) );
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
	switch( GET_DIDEVICE_TYPE( didInstance.dwDevType ) )
	{
	case DI8DEVTYPE_GAMEPAD:
		sDeviceEnum.szName = StrFmt( "GAMEPAD%d", nCounter[0] );
		nCounter[0]++;
		break;
	case DI8DEVTYPE_DRIVING:
		sDeviceEnum.szName = StrFmt( "DRIVING%d", nCounter[1] );
		nCounter[1]++;
		break;
	case DI8DEVTYPE_JOYSTICK:
		sDeviceEnum.szName = StrFmt( "JOYSTICK%d", nCounter[2] );
		nCounter[2]++;
		break;
	default:
		sDeviceEnum.szName = StrFmt( "GAMECTRL%d", nCounter[3] );
		nCounter[3]++;
		break;
	}

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
		szControlName += StrFmt( "_SLIDER%d", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Slider;

		return DIENUM_CONTINUE;
	}
	else if ( lpdidObject->guidType == GUID_Button )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += StrFmt( "_BUTTON%d", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Button;

		if( DIDFT_GETINSTANCE( lpdidObject->dwType ) > 32 )
			return DIENUM_CONTINUE;

		diObjectFormat.dwOfs = FIELD_OFFSET( SInputDataFormat, bButton[ DIDFT_GETINSTANCE( lpdidObject->dwType ) ] );
	}
	else if ( lpdidObject->guidType == GUID_Key )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += StrFmt( "_KEY%d", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Key;

		return DIENUM_CONTINUE;
	}
	else if ( lpdidObject->guidType == GUID_Unknown )
	{
		eType = CT_KEY;
		szControlName = psDeviceEnum->szName;
		szControlName += StrFmt( "_UNKNOWN%d", DIDFT_GETINSTANCE( lpdidObject->dwType ) );
		diObjectFormat.pguid = &GUID_Unknown;

		return DIENUM_CONTINUE;
	}

	diObjectFormat.dwType = lpdidObject->dwType;
	diObjectFormat.dwFlags = 0;
	psDeviceEnum->nNumControls++;
	psDeviceEnum->vectorObjects.push_back( diObjectFormat );

	SKey sKey;
	sKey.eType = eType;
	sKey.nAction = INPUT_KEYID( psDeviceEnum->nID, diObjectFormat.dwOfs );
	sKey.nDevType = GET_DIDEVICE_TYPE( diObjectFormat.dwType );

	nameIDs[szControlName] = sKey.nAction;
	idNames[ sKey.nAction ] = szControlName;
	actionIDs[ INPUT_KEYID( psDeviceEnum->nID, diObjectFormat.dwOfs ) ] = sKey;

	DebugTrace("INPUT:\tNew control found! Add new control %s\n", szControlName );

	if ( eType == CT_POV )
	{
		SKey sKey;
		sKey.eType = eType;
		sKey.nDevType = GET_DIDEVICE_TYPE( diObjectFormat.dwType );

		sKey.nAction = INPUT_KEYIDEX( psDeviceEnum->nID, diObjectFormat.dwOfs, 1 );
		sKey.ePOVAxis = PA_X;
		nameIDs[szControlName + "_X"] = sKey.nAction;
		idNames[ sKey.nAction ] = szControlName + "_X";
		actionIDs[ INPUT_KEYIDEX( psDeviceEnum->nID, diObjectFormat.dwOfs, 1 ) ] = sKey;

		DebugTrace("INPUT:\tNew control found! Add new control %s\n", szControlName + "_X" );

		sKey.nAction = INPUT_KEYIDEX( psDeviceEnum->nID, diObjectFormat.dwOfs, 2 );
		sKey.ePOVAxis = PA_Y;
		nameIDs[szControlName + "_Y"] = sKey.nAction;
		idNames[ sKey.nAction ] = szControlName + "_Y";
		actionIDs[ INPUT_KEYIDEX( psDeviceEnum->nID, diObjectFormat.dwOfs, 2 ) ] = sKey;

		DebugTrace("INPUT:\tNew control found! Add new control %s\n", szControlName + "_Y" );
	}

	return DIENUM_CONTINUE;
}

bool IsMouseDisabledDebug()
{
	return bMouseDisabledDebug;
}


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

