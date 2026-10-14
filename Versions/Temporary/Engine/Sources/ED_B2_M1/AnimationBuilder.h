#pragma once

#include "MapEditorLib/BuildDataBuilder.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

struct SGrannyBoneAttributes;

class CAnimationBuilder : public CDefaultBuilderBase, public IBuildDataCallback, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CAnimationBuilder );

	CAnimationBuilder();
	~CAnimationBuilder() override;

	bool UpdateAminations( const std::string &rszAnimationFolder );

protected:
	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
	// IBuildDataCallback
	bool
	IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView ) override;
	bool IsUniqueObjectName( const std::string &szObjectType, const std::string &szObjectName ) override;

	// methods
	uint32_t GetWeaponBits( const SGrannyBoneAttributes &gba ) const;
};
