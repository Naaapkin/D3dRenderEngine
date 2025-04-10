#include "Material.h"

bool ShaderConstant::operator==(const ShaderConstant& other) const
{
    return mRegister == other.mRegister && mName == other.mName;
}
