#include "UI/stdafx.h"
#include "UI/Window.h"

#include <gtest/gtest.h>

namespace
{
// Supply the saved button fields through its real serializer. Skipping visual
// resources keeps this regression independent of graphics and descriptor checks.
class CButtonStateSaver : public IBinSaver
{
	bool reading;
	CPtr<NDb::SWindowMSButton> instance;
	std::vector<std::pair<chunk_id, int>> chunks;

	template<class T> void Transfer( void *data, int size, T &value )
	{
		ASSERT_EQ( size, sizeof(T) );
		if ( reading )
			memcpy( data, &value, sizeof(T) );
		else
			memcpy( &value, data, sizeof(T) );
	}

	bool StartChunk( chunk_id id, int number ) override
	{
		if ( ( chunks.empty() && ( id == 2 || id == 8 ) ) ||
			( chunks.size() == 1 && chunks[0].first == 8 && id == 1 ) )
		{
			chunks.emplace_back( id, number );
			return true;
		}
		return false;
	}
	void FinishChunk() override { chunks.pop_back(); }
	int CountChunks( chunk_id ) override { return 0; }
	void DataChunk( chunk_id id, void *data, int size, int ) override
	{
		if ( chunks.size() == 1 && chunks[0].first == 8 && id == 2 )
		{
			int count = substates.size();
			Transfer( data, size, count );
			if ( !reading )
				substates.resize( count );
		}
		else if ( chunks.size() == 2 && id == 5 )
			Transfer( data, size, substates[chunks[1].second - 1] );
		else if ( chunks.empty() && id == 10 )
		{
			bool effect = false;
			Transfer( data, size, effect );
		}
		else if ( chunks.empty() && id == 12 )
			Transfer( data, size, originalSubstate );
	}
	void DataChunkString( std::string & ) override {}
	void DataChunkString( std::wstring & ) override {}
	void StoreObject( CObjectBase * ) override {}
	CObjectBase *LoadObject() override { return instance; }
	void DestroyContents() override {}

public:
	std::vector<NDb::EButtonSubstateType> substates;
	NDb::EButtonSubstateType originalSubstate = NDb::BST_NORMAL;

	CButtonStateSaver( bool read, NDb::SWindowMSButton *desc = nullptr,
		std::vector<NDb::EButtonSubstateType> states = {} )
		: reading( read ), instance( desc ), substates( std::move( states ) ) {}
	bool IsReading() override { return reading; }
	int GetVersion() const override { return 4; }
	int GetSizeOf() const override { return sizeof(*this); }
};

class CAfterLoadChild : public CWindow
{
	OBJECT_NOCOPY_METHODS( CAfterLoadChild );
	CPtr<NDb::SWindowSimple> instance =
		MakeObject<NDb::SWindowSimple>( NDb::SWindowSimple::typeID );
	NDb::SWindow *GetInstance() override { return instance; }
public:
	int afterLoadCalls = 0;
	void AfterLoad() override
	{
		++afterLoadCalls;
		CWindow::AfterLoad();
	}
};

class WindowAfterLoad : public testing::Test
{
protected:
	CObj<CWindow> button;
	CObj<CAfterLoadChild> child;
	CObj<CAfterLoadChild> grandchild;

	void SetUp() override
	{
		// CWindowMSButton's registered save/load type; no private UI exports needed.
		button = MakeObjectVirtual<CWindow>( 0x11075B87 );
		ASSERT_NE( button.GetPtr(), nullptr );
		child = new CAfterLoadChild;
		grandchild = new CAfterLoadChild;
		child->AddChild( grandchild, false );
		button->AddChild( child, false );
	}

	void RestoreAndCheckChildren()
	{
		button->AfterLoad();
		EXPECT_EQ( child->afterLoadCalls, 1 );
		EXPECT_EQ( grandchild->afterLoadCalls, 1 );
		EXPECT_EQ( child->GetParent(), button.GetPtr() );
		EXPECT_EQ( grandchild->GetParent(), child.GetPtr() );
	}
};
}

TEST_F( WindowAfterLoad, EmptySavedStatesStillRestoreChildren )
{
	CPtr<NDb::SWindowMSButton> instance =
		MakeObject<NDb::SWindowMSButton>( NDb::SWindowMSButton::typeID );
	instance->nState = 0;
	instance->buttonStates.resize( 1 );
	// A rejected logical/visual state count mismatch leaves exactly this saved
	// combination: a valid instance with one logical state and no runtime states.
	CButtonStateSaver reader( true, instance );
	(*button) & reader;
	RestoreAndCheckChildren();
	CButtonStateSaver writer( false );
	(*button) & writer;
	EXPECT_TRUE( writer.substates.empty() );
	EXPECT_EQ( instance->nState, 0 );
}

TEST_F( WindowAfterLoad, ValidSavedSubstateIsRetained )
{
	CPtr<NDb::SWindowMSButton> instance =
		MakeObject<NDb::SWindowMSButton>( NDb::SWindowMSButton::typeID );
	instance->nState = 1;
	const std::vector<NDb::EButtonSubstateType> saved =
		{ NDb::BST_MOUSE_OVER, NDb::BST_PUSHED_DEEP };
	CButtonStateSaver reader( true, instance, saved );
	(*button) & reader;
	RestoreAndCheckChildren();
	CButtonStateSaver writer( false );
	(*button) & writer;
	EXPECT_EQ( instance->nState, 1 );
	EXPECT_EQ( writer.substates, saved );
	EXPECT_EQ( writer.originalSubstate, NDb::BST_PUSHED_DEEP );
}
