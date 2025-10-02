
#pragma once

#include "RegFunction.h"
//#include "lsaver.h"

//
struct SLuaParams
{
	int n;
	float f;
	bool b;
	string s;
	CPtr<CObjectBase> p;
};
//
#define LUA_NOERR	0
const char *ErrorToString( int nErrorCode );

class Script;
extern Script* pCurrentScript;

class Script
{
	int nPushCount;
public:
	
	typedef int TableIterator;
	typedef int ObjectReference;
/*	class Reference
	{
	private:
		Script *m_parent;	// The parent script of this object.
		int m_stackIndex;	// The stack index representing this object.
	public:
		Reference( Script *pParent, int index ) : m_parent(pParent), m_stackIndex(index) {}
	};*/
	///////////////////////////////////////////////////////////////////////////
	class Object
	{
	private:
		Script *m_parent;	// The parent script of this object.
		int m_stackIndex;	// The stack index representing this object.

	public:
		Object( Script *pParent, int index ) : m_parent(pParent), m_stackIndex(index) {}
		Object() : m_parent( pCurrentScript ) {}

		int operator&( IBinSaver &f ); 

		Script& GetParent() const			{  return *m_parent;  }
		lua_State* GetState() const			{  return m_parent->m_state;  }

		int GetType() const					{  return lua_type(GetState(), m_stackIndex);  }

		bool IsNil() const					{  return m_parent->IsNil(m_stackIndex);  }
		bool IsTable() const				{  return m_parent->IsTable(m_stackIndex);  }
		bool IsUserData() const				{  return m_parent->IsUserData(m_stackIndex);  }
		bool IsCFunction() const			{  return lua_iscfunction(GetState(), m_stackIndex) != 0;  }
		bool IsNumber() const				{  return lua_isnumber(GetState(), m_stackIndex) != 0;  }
		bool IsNumberOnly() const { return lua_isnumber_only(GetState(), m_stackIndex ) != 0; }
		bool IsString() const				{  return m_parent->IsString(m_stackIndex);  }
		bool IsFunction() const				{  return m_parent->IsFunction(m_stackIndex);  }
		bool IsNull() const					{  return m_parent->IsNull(m_stackIndex);  }

		int GetStackIndex() const			{  return m_stackIndex;  }

		int GetInteger() const				{ return (int)lua_tonumber(GetState(), m_stackIndex);  }
		operator int() const				{ return GetInteger(); }
		double GetNumber() const			{ return lua_tonumber(GetState(), m_stackIndex);  }
		const char* GetString() const		{ return lua_tostring(GetState(), m_stackIndex);  }
		operator const char *() const		{ return GetString(); }
		operator string () const			{ return string( GetString() ); }
		int StrLen() const					{ return lua_strlen(GetState(), m_stackIndex);  }
		CFunction GetCFunction() const		{ return lua_tocfunction(GetState(), m_stackIndex);  }
		CObjectBase* GetUserData() const			{ return lua_touserdata(GetState(), m_stackIndex);  }
		const void* GetPointer() const		{ return lua_topointer(GetState(), m_stackIndex);  }

		// Creates a table called [name] within the current Object.
		Object CreateTable(const char* name)
		{
			int val;
			val = m_parent->GetTop();
			lua_newtable(GetState());							// T
			val = m_parent->GetTop();
			lua_pushstring(GetState(), name);					// T name
			val = m_parent->GetTop();
			lua_pushvalue(GetState(), lua_gettop(GetState()) - 1);	// T name T
			val = m_parent->GetTop();
			lua_settable(GetState(), m_stackIndex);
			val = m_parent->GetTop();

			return Object(m_parent, m_parent->GetTop());
		}
		
		// Creates (or reassigns) the object called [name] to [value].
		void SetNumber(const char* name, double value)
		{
			lua_pushstring(GetState(), name);
			lua_pushnumber(GetState(), value);
			lua_settable(GetState(), m_stackIndex);
		}
		
		// Creates (or reassigns) the object called [name] to [value].
		void SetString(const char* name, const char* value)
		{
			lua_pushstring(GetState(), name);
			lua_pushstring(GetState(), value);
			lua_settable(GetState(), m_stackIndex);
		}
		
		// Creates (or reassigns) the object called [name] to [data].
		void SetUserData( const char* name, CObjectBase* data )
		{
			lua_pushstring(GetState(), name);
			lua_pushuserdata(GetState(), data);
			lua_settable(GetState(), m_stackIndex);
		}
		// Creates (or reassigns) the object called [name] to [data].
		void SetUserTag( const char* name, CObjectBase* data, int nTag )
		{
			lua_pushstring(GetState(), name);
			lua_pushusertag(GetState(), data, nTag);
			lua_settable(GetState(), m_stackIndex);
		}
		
		int Tag() const {  return lua_tag(GetState(), m_stackIndex);  }
		bool IsUserTag() const { return Tag() > LUA_TFUNCTION; }

		// Assuming the current object is a table, retrieves the table entry called [name].
		Object GetByName( const char* name )
		{
			lua_pushstring(GetState(), name);
			lua_rawget(GetState(), m_stackIndex);
			return Object(m_parent, m_parent->GetTop());
		}

		// Assuming the current object is a table, retrieves the table entry at [index].
		Object GetByTableIndex( int index )
		{
			lua_rawgeti(GetState(), m_stackIndex, index);
			return Object(m_parent, m_parent->GetTop());
		}
		
		// Table operation ////////////////////////////////////////////////////
		int GetTableSize()		const { return lua_getn( GetState(), m_stackIndex ); }
		TableIterator First()	{ m_parent->PushNil(); m_parent->PushNil(); return Next(); }
		TableIterator Next()	{ m_parent->Pop(); return lua_next( GetState(), m_stackIndex ); }
		Object GetEntryValue( TableIterator i )	{ return m_parent->GetObject( m_parent->GetTop() ); }
		string GetEntryName( TableIterator i )	{ return m_parent->GetObject( m_parent->GetTop() - 1 ).GetString(); }

		ObjectReference GetRef( bool needLock = true )
		{
			ASSERT( m_stackIndex == m_parent->GetTop() ); // you must get reference befor other operations
			ObjectReference ref = m_parent->PopAsRef(needLock);
			m_parent->PushByRef(ref);
			return ref;
		}
	};

	///////////////////////////////////////////////////////////////////////////
	class AutoBlock
	{
	public:
		AutoBlock(Script& script) :
			m_script(script)
		{
			m_stackTop = m_script.GetTop();
		}

		AutoBlock(Object& object) :
			m_script(object.GetParent())
		{
			m_stackTop = m_script.GetTop();
		}

		~AutoBlock()
		{
			m_script.SetTop(m_stackTop);
		}

	private:
		AutoBlock(const AutoBlock& src);					// Not implemented
		const AutoBlock& operator=(const AutoBlock& src); // Not implemented

		Script& m_script;
		int m_stackTop;
	};

	///////////////////////////////////////////////////////////////////////////
	enum { NOREF = LUA_NOREF };
	enum { REFNIL = LUA_REFNIL };
	enum { ANYTAG = LUA_ANYTAG };

	Script( const char* name = 0, bool initStandardLibrary = true, lua_CFunction logFunction = 0 );
	Script(lua_State* state) : m_state(state), m_ownState(false), nPushCount( 0 ) {}
	virtual ~Script()
	{
		if (m_ownState)
			lua_close(m_state);
	}


	// Basic stack manipulation.
	int GetTop() const							{  return lua_gettop(m_state);  }
	void SetTop(int index)						{  lua_settop(m_state, index);  }
	void PushValue(int index)					{  lua_pushvalue(m_state, index);  }
	void Remove(int index)						{  lua_remove(m_state, index);  }
	void Insert(int index)						{  lua_insert(m_state, index);  }

	Object GetObject(int index)					{  return Object(this, index);  }
	Object GetTopObject()						{  return Object(this, GetTop());  }

	// access functions (stack -> C)
	int Equal(int index1, int index2)			{  return lua_equal(m_state, index1, index2);  }
	int LessThan(int index1, int index2)		{  return lua_lessthan(m_state, index1, index2);  }

	// push functions (C -> stack)
	void PushBool(bool value)					{  if (value)  lua_pushnumber(m_state, 1);  else  lua_pushnil(m_state); ++nPushCount; }
	void PushNil()								{  lua_pushnil(m_state); ++nPushCount; }
	void PushNumber(double n)					{  lua_pushnumber(m_state, n); ++nPushCount;  }
	void PushLString(const char *s, size_t len)	{  lua_pushlstring(m_state, s, len); ++nPushCount;  }
	void PushString(const char *s)				{  lua_pushstring(m_state, s); ++nPushCount;  }
	void PushCClosure(lua_CFunction fn, int n)	{  lua_pushcclosure(m_state, fn, n); ++nPushCount;  }
	int GetPushCount() const { return nPushCount; }

	// get functions (Lua -> stack)
	Object GetGlobal(const char *name)			{  lua_getglobal(m_state, name);  return Object(this, GetTop());  }
	void GetTable(int index)					{  lua_gettable(m_state, index);  }
	void RawGet(int index)						{  lua_rawget(m_state, index);  }
	void RawGetI(int index, int n)				{  lua_rawgeti(m_state, index, n);  }
	Object GetGlobals()							{  lua_getglobals(m_state);  return Object(this, GetTop());  }
	void GetTagMethod(int tag, const char *event)	{  lua_gettagmethod(m_state, tag, event);  }

	Object NewTable()							{ lua_newtable(m_state);  return Object(this, GetTop());  }

	// set functions(stack -> Lua)
	void SetGlobal(const char *name)			{  lua_setglobal(m_state, name);  }
	void SetTable(int index)					{  lua_settable(m_state, index);  }
	void RawSet(int index)						{  lua_rawset(m_state, index);  }
	void RawSetI(int index, int n)				{  lua_rawseti(m_state, index, n);  }
	void SetGlobals()							{  lua_setglobals(m_state);  }
	void SetTagMethod(int tag, const char *event)	{  lua_settagmethod(m_state, tag, event);  }

	// References
	ObjectReference PopAsRef( bool needLock = true ){ return lua_ref( m_state, needLock );  }
	bool PushByRef( ObjectReference ref )			{ return lua_getref( m_state, ref );  } // return false, if the object has been garbage collected
	void ReleaseRef( ObjectReference ref )			{ lua_unref( m_state, ref ); }
	Object GetRegestryTable()						{ lua_getregistry(m_state); return Object(this, GetTop()); }

	// "do" functions(run Lua code)
	int DoFile(const char *filename)			{  return lua_dofile(m_state, filename);  }
	int DoString(const char *str)				{  return lua_dostring(m_state, str);  }
	int DoBuffer(const char *buff, size_t size, const char *name = 0)	{  return lua_dobuffer(m_state, buff, size, name);  }
	int ParseBuffer(const char *buff, size_t size, const char *name = 0)	{  return lua_parsebuffer(m_state, buff, size, name);  }
	void ExecuteThreads( void ) { lua_executeThreads( m_state ); }

	// miscellaneous functions
	int NewTag()								{ return lua_newtag(m_state);  }
	int CopyTagMethods(int tagto, int tagfrom)	{ return lua_copytagmethods(m_state, tagto, tagfrom);  }
	void SetTag(int tag)						{ lua_settag(m_state, tag);  }

	void Error(const char *s)					{ lua_error(m_state, s);  }

	void Unref(int ref)							{ lua_unref(m_state, ref);  }

	void ConcatenateStrings(int n)				{  lua_concat(m_state, n);  }

	// Helper function
	void Pop(int amount = 1)					{  lua_pop(m_state, amount);  }
	bool CheckArgs( const char *szArgList, const string &sFuncName, vector<SLuaParams> *pParams, bool bLog );

	
	int  RegisterNewTag( const SRegFunction *pList );
	void Register( const SRegFunction *pList );
	void RegisterSaveLoad( const SRegFunction *pList );
	void Register( const char* funcName, lua_CFunction function );
	void PushUserTag( CObjectBase* u, int nTag )	{  lua_pushusertag( m_state, u, nTag );  }
	void PushUserData( CObjectBase* u )			{  lua_pushuserdata(m_state, u);  }
	void PushCFunction(lua_CFunction f)		{  lua_pushcclosure(m_state, f, 0);  }
	int  CloneTag(int t)					{  return lua_copytagmethods(m_state, lua_newtag(m_state), t);  }

	bool IsFunction(int index)					const {  return lua_isfunction(m_state, index);  }
	bool IsString(int index)					const {  return lua_isstring(m_state, index) != 0;  }
	bool IsTable(int index)						const {  return lua_istable(m_state, index);  }
	bool IsUserData(int index)					const {  return lua_isuserdata(m_state, index);  }
	bool IsNil(int index)						const {  return lua_isnil(m_state, index);  }
	bool IsNull(int index)						const {  return lua_isnull(m_state, index);  }

	int ConfigGetInteger(const char* section, const char* entry, int defaultValue = 0);
	float ConfigGetReal(const char* section, const char* entry, double defaultValue = 0.0);
	const char* ConfigGetString(const char* section, const char* entry, const char* defaultValue = "");
	void ConfigSetInteger(const char* section, const char* entry, int value);
	void ConfigSetReal(const char* section, const char* entry, double value);
	void ConfigSetString(const char* section, const char* entry, const char* value);

	string GetObjectAsText( const char* name, bool bDecorateOutput );
	string GetObjectAsText( const char* name, Object value, bool bDecorateOutput, unsigned int indentLevel = 0 );
	string GetStateAsText();

	operator lua_State*()						{  return m_state;  }
	lua_State* GetState() const					{  return m_state;  }

public:
	friend class Object;

	lua_State* m_state;
	bool m_ownState;
	int operator&( IBinSaver &f );
};

