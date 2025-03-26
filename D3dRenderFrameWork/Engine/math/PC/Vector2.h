#pragma once
#ifdef WIN32
#include <Engine/pch.h>
struct Vector2
{
    DirectX::XMFLOAT2 v;
    //
    // static float Length(const Vector2& v);
    // static float LengthSquared(const Vector2& v);
    // static Vector2 Dot(const Vector2& v1, const Vector2& v2);
    // static float Distance(const Vector2& v1, const Vector2& v2);
    // static void Normalize(Vector2& v);
    //
    // float Length() const;
    // float LengthSquared() const;
    // void Dot(const Vector2& rhs) const;
    // Vector2 Normalize() const;
    // Vector2() : v(DirectX::XMFLOAT2(0, 0)) { }
    // Vector2(const DirectX::XMFLOAT2& v) : v(v) { }
    // Vector2(float x, float y) : v(DirectX::XMFLOAT2(x, y)) { }
    //
    // float operator[](int index) const;
    // Vector2 operator+(float scalar) const;
    // Vector2 operator-(float scalar) const;
    // Vector2 operator*(float scalar) const;
    // Vector2 operator/(float scalar) const;
    // Vector2 operator+(const Vector2& rhs) const;
    // Vector2 operator-(const Vector2& rhs) const;
    // Vector2 operator*(const Vector2& rhs) const;
    // Vector2 operator/(const Vector2& rhs) const;
    // Vector2& operator+=(const Vector2& rhs);
    // Vector2& operator-=(const Vector2& rhs);
    // Vector2& operator*=(const Vector2& rhs);
    // Vector2& operator/=(const Vector2& rhs);
    // Vector2& operator+=(float scalar);
    // Vector2& operator-=(float scalar);
    // Vector2& operator*=(float scalar);
    // Vector2& operator/=(float scalar);
    // bool operator==(const Vector2& rhs) const;
    // bool operator!=(const Vector2& rhs) const;
};
#endif