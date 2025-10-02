
#pragma once

class CGroupUnit
{
	public: int operator&( IBinSaver &saver ); private:;
	
	// номер группы
	int nGroup;
	// позиция в списке юнитов группы
	int nPos;
	// номер подгруппы ( для ходьбы с сохранением относительной позиции ); -1 - сам по себе
	int nSubGroup;
	// смещение относительно центра группы
	CVec2 vShift;
	
	// номер AI группы ( н-р, для ambush )
	int nSpecialGroup;
	int nSpecialPos;

public:
	CGroupUnit() : nGroup( 0 ), nPos( 0 ), nSubGroup( -1 ), vShift( VNULL2 ), nSpecialGroup( 0 ), nSpecialPos( -1 ) { }
	void Init() {}

	const CVec2& GetGroupShift() const { return vShift; }
	const int& GetSubGroup() const { return nSubGroup; }
	const int GetNGroup() const { return nGroup; }
	const int GetSpecialGroup() const { return nSpecialGroup; }

	void SetGroupShift( const CVec2 &_vShift ) { vShift = _vShift; }	
	void SetSubGroup( const int _nSubGroup ) { nSubGroup = _nSubGroup; }

	friend class CGroupLogic;
};


