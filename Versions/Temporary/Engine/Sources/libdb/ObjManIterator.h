#pragma once

#include "TypeDef.h"

namespace NDb
{

struct IObjManIterator : public CObjectBase
{
	// get base name (full field name = BaseName + '.' + GetName())
	virtual std::string GetBaseName() const = 0;
	// goto next field
	virtual bool Next() = 0;
	// have we reached end?
	virtual bool IsEnd() const = 0;
	// get current field name
	virtual std::string GetName() const = 0;
	// get current field descriptor
	virtual const NTypeDef::STypeStructBase::SField *GetDesc() const = 0;
};

}
