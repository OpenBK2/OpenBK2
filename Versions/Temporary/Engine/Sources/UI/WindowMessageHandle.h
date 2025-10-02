// WindowMessageHandle.h: interface for the WindowMessageHandle class.
//


#if !defined(AFX_WINDOWMESSAGEHANDLE_H__733B319B_FB3A_4022_B18F_2E2596A5F251__INCLUDED_)
#define AFX_WINDOWMESSAGEHANDLE_H__733B319B_FB3A_4022_B18F_2E2596A5F251__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef bool (*MESSAGE_HANDLER)( const struct SBUIMessage &msg, void *pObj );

class CUIMessageHandler
{
	MESSAGE_HANDLER pH;										// message handler to call
public:
	CUIMessageHandler() : pH( 0 ) {  }
	CUIMessageHandler( MESSAGE_HANDLER _pH ) : pH( _pH ) {  }
	bool Execute( const struct SBUIMessage &msg, void *pObj ) 
	{ 
		NI_ASSERT( pH != 0, "null message handler" );
		return pH( msg, pObj ); 
	}
};

// handle map is used by all macros
#define HM_TYPE hash_map<string,CUIMessageHandler>
#define DECLARE_HANDLE_MAP static HM_TYPE handleMap;
#define IMPLEMENT_HANDLE_MAP(ClassName) HM_TYPE ClassName::handleMap; 

// message handler declaration 
#define DECLARE_MESSAGE_HANDLER(HandleName) \
		static bool HandleName( const struct SBUIMessage &msg, void *pObj );\
		bool _##HandleName( const struct SBUIMessage &msg );

// to use write
// IMPLEMENT_MESSAGE_HANDLER(CWindow,MouseMove)
// {
//    function body
// }
#define IMPLEMENT_MESSAGE_HANDLER(ClassName,HandleName) \
	bool ClassName::HandleName( const struct SBUIMessage &msg, void *pObj )\
	{\
		return ((ClassName*)(pObj))->_##HandleName( msg );\
	}\
	bool ClassName::_##HandleName( const struct SBUIMessage &msg )
		

// messag handler registratoin
//#define REGISTER_MESSAGE_HANDLER(ClassName,HandleName,MessageName)\
	//handleMap["MessageName"] = CUIMessageHandler( ClassName::HandleName );	


#endif // !defined(AFX_WINDOWMESSAGEHANDLE_H__733B319B_FB3A_4022_B18F_2E2596A5F251__INCLUDED_)

