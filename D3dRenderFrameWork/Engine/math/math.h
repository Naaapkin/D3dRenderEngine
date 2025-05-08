#pragma once
#ifdef WIN32
#include <Engine/math/PC/Vector2.h>
#include <Engine/math/PC/Vector3.h>

union Float2 final
{
    struct { float x, y; };
    struct { float u, v; };
    struct { float r, g; };

    Float2() : x(0.0f), y(0.0f) {}
    Float2(float _x, float _y) : x(_x), y(_y) {}

    // 拷贝构造
    Float2(const Float2& other) : x(other.x), y(other.y) {}

    // 拷贝赋值
    Float2& operator=(const Float2& other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }
};


union Float3 final
{
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { float u, v, w; };

    Float3() : x(0.0f), y(0.0f), z(0.0f) {}
    Float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // 拷贝构造
    Float3(const Float3& other) : x(other.x), y(other.y), z(other.z) {}

    // 拷贝赋值
    Float3& operator=(const Float3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
};

union Float4 final
{
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    struct { float u, v; }; // 仅部分字段

    Float4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Float4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

    // 拷贝构造
    Float4(const Float4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    // 拷贝赋值
    Float4& operator=(const Float4& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
};

union Matrix4x4 final
{
    Float4 rows[4];   // 行访问
    float m[4][4];    // m[row][col] 访问
    float data[16];   // 连续数组，便于上传到 GPU

    Matrix4x4()
    {
        for (int i = 0; i < 4; ++i)
            rows[i] = Float4(0, 0, 0, 0);
    }

    Matrix4x4(const Matrix4x4& other)
    {
        for (int i = 0; i < 4; ++i)
            rows[i] = other.rows[i];
    }

#ifdef WIN32
    Matrix4x4(const DirectX::XMMATRIX& xm)
    {
	    DirectX::XMFLOAT4X4 temp;
        XMStoreFloat4x4(&temp, xm);
        memcpy(data, &temp, sizeof(float) * 16);
    }

    operator DirectX::XMMATRIX() const
    {
        return XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(data));
    }
#endif

    Matrix4x4& operator=(const Matrix4x4& other)
    {
        if (this != &other)
        {
            memcpy(this, &other, sizeof(Matrix4x4));
        }
        return *this;
    }

    static Matrix4x4 Identity()
    {
        Matrix4x4 mat;
        mat.m[0][0] = 1.0f;
        mat.m[1][1] = 1.0f;
        mat.m[2][2] = 1.0f;
        mat.m[3][3] = 1.0f;
        return mat;
    }

    Float4& operator[](size_t i) { return rows[i]; }
    const Float4& operator[](size_t i) const { return rows[i]; }
};

#else
#endif