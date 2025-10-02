#pragma once

class CMapObj;

class CAllAnimationsPlayer : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CAllAnimationsPlayer );
	
	struct SAnimationInfo
	{
		ZDATA
			int nAnimation;
			NTimer::STime nStartNextAnimTime;
			bool bLooped;
		ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&nAnimation); f.Add(3,&nStartNextAnimTime); f.Add(4,&bLooped); return 0; } private:
	public:
		SAnimationInfo() : nAnimation( -1 ), nStartNextAnimTime( 0 ), bLooped( false ) { }
	};
	ZDATA
		hash_map<int, CObj<CMapObj> > objects;
		hash_map<int, SAnimationInfo> playingAnimations;
		bool bSwitchToNextAnimation;
	ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&objects); f.Add(3,&playingAnimations); f.Add(4,&bSwitchToNextAnimation); return 0; } private:
public:
	CAllAnimationsPlayer() { }
	explicit CAllAnimationsPlayer( const hash_map<int, CObj<CMapObj> > &objects );

	void Update();
	void SwitchToNextAnimation() { bSwitchToNextAnimation = true; }
};

