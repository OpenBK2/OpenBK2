#ifndef __UNITS_SEGMENTS_H__
#define __UNITS_SEGMENTS_H__

#pragma ONCE

class CCommonUnit;
class CAIUnit;

class CFreezeSegments
{
	class CCommonUnit *pUnit;
public:
	typedef CCommonUnit* TObjType;

	CFreezeSegments() : pUnit( 0 ) { }

	bool ShouldBeUnregistered() const { return false; }

	void SetSegmentObject( CCommonUnit *pUnit );
	bool Check();
	const NTimer::STime ProcessSegment();
};

class CStateSegments
{
	class CCommonUnit *pUnit;

	bool bCheck;
	bool bIsValid;
public:
	typedef CCommonUnit* TObjType;

	CStateSegments() : pUnit( 0 ), bCheck( false ) { }

	bool ShouldBeUnregistered() const;

	void SetSegmentObject( CCommonUnit *pUnit );
	bool Check();
	const NTimer::STime ProcessSegment();
};

class CFirstPathSegments
{
	class CAIUnit *pUnit;
public:
	typedef CAIUnit* TObjType;

	CFirstPathSegments() : pUnit( 0 ) { }

	bool ShouldBeUnregistered() const { return false; }

	void SetSegmentObject( CAIUnit *pUnit );
	bool Check();
	const NTimer::STime ProcessSegment();
};

class CSecondPathSegments
{
	class CAIUnit *pUnit;
public:
	typedef CAIUnit* TObjType;

	CSecondPathSegments() : pUnit( 0 ) { }

	bool ShouldBeUnregistered() const { return false; }

	void SetSegmentObject( CAIUnit *pUnit );
	bool Check();
	const NTimer::STime ProcessSegment();
};

class CStayTimeSegments
{
	class CAIUnit *pUnit;
public:
	typedef CAIUnit* TObjType;

	CStayTimeSegments() : pUnit( 0 ) { }

	void SetSegmentObject( CAIUnit *pUnit );
	bool Check();
	void ProcessSegment();
};

#endif // __UNITS_SEGMENTS_H__
