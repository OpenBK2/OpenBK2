#pragma once

namespace NDb
{
namespace NObjectIDAllocator
{

void SetObjectRecordIDsFolderName( const string &szFolderName );
int AllocateNewObjectID( const string &szClassTypeName );

}
}
