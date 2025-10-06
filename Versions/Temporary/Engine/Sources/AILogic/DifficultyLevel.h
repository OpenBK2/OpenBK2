
#pragma once

class CDifficultyLevel
{
	enum EModificators
	{
		EM_SMALL_AABB = 0,
		EM_PIERCING = 1,
		EM_DAMAGE = 2,
		EM_ROTATE_SPEED = 3,
		EM_DISPERSION = 4,

		EM_MAX_NUM = 5,
	};
	ZDATA
		ZONSERIALIZE
	int nLevel;
	int nCheatLevel;



	std::vector< CArray2D<float> > coeff;

	std::vector<std::string> levelsNames;
	std::vector<std::string> coeffNames;
	std::vector<std::string> partiesNames;
public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(2,&nLevel); f.Add(3,&nCheatLevel); f.Add(4,&coeff); f.Add(5,&levelsNames); f.Add(6,&coeffNames); f.Add(7,&partiesNames); return 0; }
		void OnSerialize( IBinSaver &f );
public:
	CDifficultyLevel() : nLevel( 1 ), nCheatLevel( 255 ) { }
	void Clear() { nCheatLevel = 255; }

	virtual void Init();
	void SetLevel( const int _nLevel );
	void SetCheatLevel( const int _nCheatLevel ) { nCheatLevel = _nCheatLevel; }

	const int GetLevel() const { return (std::min)( nLevel, nCheatLevel ); }
	/*
	const float GetSmallAABBCoeff( const int nParty ) const		{ return coeff[nParty][GetLevel()][EM_SMALL_AABB]; }
	const float GetPiercingCoeff( const int nParty ) const		{ return coeff[nParty][GetLevel()][EM_PIERCING]; }
	const float GetDamageCoeff( const int nParty ) const			{ return coeff[nParty][GetLevel()][EM_DAMAGE]; }
	const float GetRotateSpeedCoeff( const int nParty ) const { return coeff[nParty][GetLevel()][EM_ROTATE_SPEED]; }
	const float GetDispersionCoeff( const int nParty ) const	{ return coeff[nParty][GetLevel()][EM_DISPERSION]; }
	*/
};


