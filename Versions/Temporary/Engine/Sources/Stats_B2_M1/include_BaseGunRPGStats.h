
WORD wDirection;
void ToAIUnits( bool bInEditor )
{
	wDirection = fDirection;
	//wDirection = WORD( fDirection * 65536.0f / 360.0f );
}

virtual const CVec3 GetShootPointPos() const { return VNULL3; }

