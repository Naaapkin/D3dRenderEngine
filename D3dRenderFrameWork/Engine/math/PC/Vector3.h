#pragma once
#ifdef WIN32
#include "pch.h"
struct Vector3
{
    DirectX::XMFLOAT3 v;

    static float Length(const Vector3& v);
    static float LengthSquared(const Vector3& v);
    static Vector3 Dot(const Vector3& v1, const Vector3& v3);
    static float Distance(const Vector3& v1, const Vector3& v3);
    static void Normalize(Vector3& v);
    
    float Length() const;
    float LengthSquared() const;
    void Dot(const Vector3& rhs) const;
    Vector3 Normalize() const;
    Vector3() : v(DirectX::XMFLOAT3(0, 0, 0)) { }
    Vector3(const DirectX::XMFLOAT3& v) : v(v) { }
    Vector3(float x, float y, float z) : v(DirectX::XMFLOAT3(x, y, z)) { }
    
    float operator[](int index) const;
    Vector3 operator+(float scalar) const;
    Vector3 operator-(float scalar) const;
    Vector3 operator*(float scalar) const;
    Vector3 operator/(float scalar) const;
    Vector3 operator+(const Vector3& rhs) const;
    Vector3 operator-(const Vector3& rhs) const;
    Vector3 operator*(const Vector3& rhs) const;
    Vector3 operator/(const Vector3& rhs) const;
    Vector3& operator+=(const Vector3& rhs);
    Vector3& operator-=(const Vector3& rhs);
    Vector3& operator*=(const Vector3& rhs);
    Vector3& operator/=(const Vector3& rhs);
    Vector3& operator+=(float scalar);
    Vector3& operator-=(float scalar);
    Vector3& operator*=(float scalar);
    Vector3& operator/=(float scalar);
    bool operator==(const Vector3& rhs) const;
    bool operator!=(const Vector3& rhs) const;
};
#endif