#pragma once

#include "Mosaic/Parameter.h"
#include "Mosaic/Config.h"

namespace Mosaic::Helpers
{
	extern VTMOSAIC_API std::string GetTypeNameFromTypeInfo(const TypeInfo& typeInfo);
	extern VTMOSAIC_API TypeInfo GetPromotedTypeInfo(const TypeInfo& A, const TypeInfo& B);
}
