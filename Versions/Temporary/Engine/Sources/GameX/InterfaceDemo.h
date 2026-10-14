#pragma once

#include "InterfaceScreenBase.h"

struct IXmlSaver;
class CBackgroundMutableTexture;

class CInterfaceDemo : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceDemo );

	struct SFrame
	{
		ZDATA
		std::string szFileName;
		float fDelay;

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &szFileName );
			f.Add( 3, &fDelay );
			return 0;
		}

		int operator&( IXmlSaver &saver );
	};

	ZDATA_( CInterfaceScreenBase )
	CPtr<IWindow> pMain;
	std::vector<SFrame> frames;
	float fDelay;
	bool bFinished;
	CObj<CBackgroundMutableTexture> pPictureTexture;
	int nNextPicture;
	bool bFinal;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &pMain );
		f.Add( 3, &frames );
		f.Add( 4, &fDelay );
		f.Add( 5, &bFinished );
		f.Add( 6, &pPictureTexture );
		f.Add( 7, &nNextPicture );
		f.Add( 8, &bFinal );
		return 0;
	}

	NTimer::STime timeAbs; // don't save
private:
	void MakeInterior();
	void LoadSequence();
	bool LoadImage( const std::string &szFileName );
	void NextPicture();

public:
	CInterfaceDemo();

	bool Init() override;

	bool StepLocal( bool bAppActive ) override;

	void SetFinal( bool _bFinal );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;
	//}
};

class CICInterfaceDemo : public CInterfaceCommandBase<CInterfaceDemo>
{
	OBJECT_BASIC_METHODS( CICInterfaceDemo );

	ZDATA_( CInterfaceCommandBase<CInterfaceDemo> )
	bool bFinal;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceCommandBase<CInterfaceDemo> *)this );
		f.Add( 2, &bFinal );
		return 0;
	}

private:
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
