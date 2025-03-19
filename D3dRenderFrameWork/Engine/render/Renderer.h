#pragma once
#include <Engine/pch.h>

class Renderer
{
public:
    virtual ~Renderer();
    
    DELETE_COPY_CONSTRUCTOR(Renderer)
    DELETE_COPY_OPERATOR(Renderer)
    DEFAULT_MOVE_CONSTRUCTOR(Renderer)
    DEFAULT_MOVE_OPERATOR(Renderer)

protected:
    Renderer() = default;
};

Renderer* gRenderer();