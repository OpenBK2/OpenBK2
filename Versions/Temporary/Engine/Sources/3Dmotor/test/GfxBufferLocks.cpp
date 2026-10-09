// Exercise real buffer-lock bookkeeping with a fake D3D buffer; no GPU is needed.
#include "3Dmotor/stdafx.h"
#include "3Dmotor/GfxBuffersInternal.h"

#include <gtest/gtest.h>

namespace NGfx
{
// CRBase is instantiated here, so its non-exported bookkeeping flag belongs to
// the test executable rather than the renderer DLL.
bool bWasLinearBufferLock = false;
}

namespace
{
class CTestD3DBuffer
{
public:
	HRESULT result = D3D_OK;
	int lockCalls = 0;
	int unlockCalls = 0;
	unsigned char data[16] = {};
	void AddRef() {}
	void Release() {}
	HRESULT Lock( UINT, UINT, void **ppData, DWORD )
	{
		++lockCalls;
		// A failed API call may have touched its output. It must not become the
		// published pointer or be unlocked by cleanup when the HRESULT fails.
		*ppData = data;
		return result;
	}
	HRESULT Unlock() { ++unlockCalls; return D3D_OK; }
};

class CTestVertexBuffer : public NGfx::CRBase<CTestD3DBuffer>
{
	OBJECT_NOCOPY_METHODS( CTestVertexBuffer );
};
}

TEST( GfxBufferLocks, FailedLockDoesNotPublishPointerOrIncrementCount )
{
	CTestD3DBuffer deviceBuffer;
	deviceBuffer.result = D3DERR_DEVICELOST;
	CTestVertexBuffer buffer;
	buffer.obj = &deviceBuffer;
	NGfx::bWasLinearBufferLock = false;
	try
	{
		buffer.Lock( D3DLOCK_DISCARD );
		FAIL() << "A failed D3D lock must throw";
	}
	catch ( const std::runtime_error &error )
	{
		EXPECT_NE( std::string( error.what() ).find( "88760868" ), std::string::npos );
	}
	EXPECT_EQ( buffer.nLockCount, 0 );
	EXPECT_EQ( buffer.pLocked, nullptr );
	EXPECT_FALSE( NGfx::bWasLinearBufferLock );
	buffer.Free();
	EXPECT_EQ( deviceBuffer.lockCalls, 1 );
	EXPECT_EQ( deviceBuffer.unlockCalls, 0 );
}

TEST( GfxBufferLocks, SuccessfulNestedLocksShareOneDeviceLock )
{
	CTestD3DBuffer deviceBuffer;
	CTestVertexBuffer buffer;
	buffer.obj = &deviceBuffer;
	buffer.Lock( D3DLOCK_DISCARD );
	buffer.Lock( D3DLOCK_NOOVERWRITE );
	EXPECT_EQ( buffer.nLockCount, 2 );
	EXPECT_EQ( buffer.pLocked, deviceBuffer.data );
	EXPECT_EQ( deviceBuffer.lockCalls, 1 );
	buffer.Unlock();
	buffer.Unlock();
	buffer.Free();
	EXPECT_EQ( buffer.pLocked, nullptr );
	EXPECT_EQ( deviceBuffer.unlockCalls, 1 );
}

TEST( GfxBufferLocks, MissingResourcesThrowBeforeDereferencing )
{
	CTestVertexBuffer vertexBuffer;
	NGfx::CIB16Fast indexBuffer;
	EXPECT_THROW( vertexBuffer.Lock( 0 ), std::runtime_error );
	EXPECT_EQ( vertexBuffer.nLockCount, 0 );
	EXPECT_THROW( indexBuffer.Lock( 0 ), std::runtime_error );
	EXPECT_EQ( indexBuffer.pLocked, nullptr );
}
