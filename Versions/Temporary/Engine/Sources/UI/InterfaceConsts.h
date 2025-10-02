// SInterfaceConsts.h: interface for the SInterfaceConsts class.
//


#if !defined(AFX_SINTERFACECONSTS_H__87DEE1F5_7500_4647_9FA9_5216B18EE536__INCLUDED_)
#define AFX_SINTERFACECONSTS_H__87DEE1F5_7500_4647_9FA9_5216B18EE536__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CInterfaceConsts  
{
public:
	CInterfaceConsts();
	virtual ~CInterfaceConsts();


	static const int SCROLLER_ANIM_TIME()
	{
		return 10;
	}
	static const int SCROLLER_ANIM_TIME_1()
	{
		return 100;
	}
	static const int CURSOR_ANIMATION_TIME()
	{
		return 500;
	}
	
	static const int LIST_CONTROL_COLUMN_HEADER_MIN_WIDTH()
	{
		return 10;
	}
	static const int LIST_CONTROL_HEADER_RESIZE_OFFSET()
	{
		return 10;
	}
	static const int CURSOR_MODE_LIST_HEADER_RESIZE()
	{
		return 1;
	}
	// to resize from 0 width
	static const int CURSOR_MODE_LIST_HEADER_RESIZE_2()
	{
		return 0;
	}
	static const int CURSOR_MODE_NORMAL_UI()
	{
		return 0;
	}
	
	// tooltip text horisontal size / vertical size
	static const float TOOLTIP_WINDOW_PROPORTION()
	{
		return 2;
	}
};

#endif // !defined(AFX_SINTERFACECONSTS_H__87DEE1F5_7500_4647_9FA9_5216B18EE536__INCLUDED_)
