#include "../game.h"
[hExternal] #include "BinaryFlags.h"

//
[typeRename = "CBinaryFlags"; numBytes = 8 ]
[noHeader] typedef hexbinary Flags;

[typeID = 0x1019230D]
class CWeapon
{
	// "How many bullets are shooted during one burst"
	int _AmmoPerBurst;
};

class CHPObject
{
	// Object's internal name
	wstring _Name;
	// Object's hit points (health)
	float _HP;
	// Does this object have passability?
	bool _HasPassability;
	// General-purpose flags
	Flags _Flags;
	// Designer, who created this object
	string _DesignerName;
};

class CUnitBase : public CHPObject
{
	enum EUnitType
	{
		UNIT_TYPE_UNKNOWN,
		UNIT_TYPE_INFANTRY_SNIPER,
		UNIT_TYPE_ARMOR_MEDIUM,
		UNIT_TYPE_ARMOR_HEAVY,
		UNIT_TYPE_AVIA_FIGHTER,
		UNIT_TYPE_AUTO_ENGINEER,
		UNIT_TYPE_SPG_ASSAULT,
	};
	// Unit type (for designers)
	EUnitType _UnitType;
	// (meters) Sight radius
	float _Sight;
	// (meters per second) Maximum speed
	float _Speed;
	// (meters) Radius for passability calculations
	int _BoundTileRadius;
};

[typeID = 0x1019230E]
class CMechUnit : public CUnitBase
{
	struct SJogging
	{
		// Cosine amplitude
		float _Amplitude;
		// Cosine phase
		float _Phase;
		// Cosine phase shift
		float _Shift;
		// Uniform tremble along all 3 axises
		CVec3 _Tremble;
	};
	//
	struct SStruct1
	{
		int _TypeInt;
		[hidden] float _TypeFloat;
		bool _TypeBool;
		[hidden] GUID _TypeGUID;
		[hidden] string _TypeString;
		wstring _TypeWString = L"Очень клёвое default value";
		EUnitType _TypeEnumUnitType;
		Flags _TypeBinaryFlags;
	};
	//
	struct SStruct2
	{
		// array of complex struct (for array-in-array testing)
		SStruct1 _Structs[];
		// array of GUIDs (for array-in-array testing)
		GUID _guids[];
	};
	//
	SJogging _Jx;
	SJogging _Jy;
	GUID _guid;
	//
	[noCode] int _NocodeInt;
	[noCode] float _nocodeFloat;
	[noCode] bool _nocodeBool;
	[noCode] string _nocodeString;
	[noCode] wstring _nocodeWString;
	[noCode] GUID _nocodeGUID;
	[noCode] EUnitType _nocodeEnumUnitType;
	[noCode] Flags _nocodeBinaryFlags;
	//
	int _SimpleArrayInt[];
	float _SimpleArrayFloat[];
	GUID _SimpleArrayGUID[];
	Flags _SimpleArrayBinaryFlags[];
	EUnitType _SimpleArrayEnumUnitType[];
	string _SimpleArrayString[];
	wstring _SimpleArrayWString[];
	//
	[noCode] int _SimpleArrayIntNocode[];
	[noCode] float _SimpleArrayFloatNocode[];
	[noCode] GUID _SimpleArrayGUIDNocode[];
	[noCode] Flags _SimpleArrayBinaryFlagsNocode[];
	[noCode] EUnitType _SimpleArrayEnumUnitTypeNocode[];
	[noCode] string _SimpleArrayStringNocode[];
	[noCode] wstring _SimpleArrayWStringNocode[];
	//
	SStruct1 _ComplexArrayStruct1[];
	SStruct2 _ComplexArrayStruct2[];
	//
	CWeapon *_Weapon;
	CWeapon *_Weapons[];
};

[typeID = 0x101A6C80]
class CMapInfo2
{
	struct SMapObject
	{
		// (проценты от 0 до 1) Жизнь
		float _HP = 1;
		// (метры) Положение на карте
		CVec3 _Pos;
		// (кватернион) Ориентация объекта в пространстве
		CQuat _Rot;
		// Уникальный идентификатор объекта на карте
		GUID _LinkID;
		// С кем объект слинкован
		GUID _LinkWith;
		// Ссылка на описатель статсов объекта
		CHPObject *_Object;
	};
	// Список всех объектов на карте
	SMapObject _Objects[];
};

