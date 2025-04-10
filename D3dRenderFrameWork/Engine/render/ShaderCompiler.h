#pragma once
#include "Engine/pch.h"

class ShaderCompiler : NonCopyable
{
public:
    static ShaderCompiler& getOrInitializeCompiler();
    
    ShaderReflector& getReflector(const String& name);

    ShaderCompiler() = default;
    virtual void compileShader(const ShaderSourceCode& sourceCode, Shader& shader) = 0;
    virtual ~ShaderCompiler() = 0;

    DELETE_COPY_CONSTRUCTOR(ShaderCompiler);
    DELETE_MOVE_CONSTRUCTOR(ShaderCompiler);
    DELETE_COPY_OPERATOR(ShaderCompiler);
    DELETE_MOVE_OPERATOR(ShaderCompiler);
    
protected:
    std::unordered_map<String, ShaderReflector> mReflectors;
    
};