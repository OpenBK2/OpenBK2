#pragma once

namespace NDb
{
namespace NObjectIDAllocator
{

void SetObjectRecordIDsFolderName( const std::string &szFolderName );
int AllocateNewObjectID( const std::string &szClassTypeName );

}
}
