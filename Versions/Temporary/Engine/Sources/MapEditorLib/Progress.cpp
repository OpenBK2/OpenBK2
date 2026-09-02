#include "stdafx.h"

#include "Interface_MainFrame.h"
#include "Interface_Progress.h"


namespace NProgress
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Create( bool bShow )
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->CreateProgressDialog();
			if ( bShow )
			{
				Singleton<IMainFrameContainer>()->Get()->CreateProgressDialog();
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Destroy()
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->DestroyProgressDialog();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetTitle( const std::string &rszTitle )
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->SetProgressDialogTitle( rszTitle );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetMessage( const std::string &rszMessage )
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->SetProgressDialogMessage( rszMessage );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetRange( int nStart, int nFinish )
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->SetProgressDialogRange( nStart, nFinish );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetPosition( int nPosition )
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->SetProgressDialogPosition( nPosition );
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void IteratePosition()
	{
		if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
		{
			Singleton<IMainFrameContainer>()->Get()->IterateProgressDialogPosition();
		}
	}
}

// basement storage  


