#pragma once

template<class T>
struct TVec2
{
	union {
		struct { T x, y; };
		struct { T r, g; };
		struct { T s, t; };
	};

	constexpr TVec2() : x(0), y(0) {}
	constexpr TVec2(T a, T b) : x(a), y(b) {}

	constexpr TVec2 operator*(T scalar)
	{
		return TVec2{ x *scalar, y * scalar };
	}
};

typedef TVec2<int> iVec2;
typedef TVec2<float> fVec2;
typedef TVec2<double> dVec2;
typedef TVec2<unsigned> uVec2;

template<class T>
inline constexpr int sgn(T val)
{
	return val < T(0) ? -1 : 1;
}

template<class T>
inline constexpr T dot(const T & xA, const T & yA, const T & xB, const T & yB)
{
	return xA*xB + yA*yB;
}

template<class T>
inline constexpr T length(const T & x, const T & y)
{
	static_assert(std::numeric_limits<T>::is_iec559, "'length' accepts only floating-point inputs");
	return sqrt(dot(x, y, x, y));
}

template<class T>
inline constexpr T distance(const T & xA, const T & yA, const T & xB, const T & yB)
{
	return length(xB - xA, yB - yA);
}

template<class T>
inline constexpr T normalizeX(const T & xA, const T & yA)
{
	return xA / length(xA, yA);
}

template<class T>
inline constexpr T normalizeY(const T & xA, const T & yA)
{
	return yA / length(xA, yA);
}

template <class T>
inline constexpr T normalize(const T & x)
{
	static_assert(std::numeric_limits<T>::is_iec559, "'normalize' accepts only floating-point inputs");
	return x < T(0) ? T(-1) : T(1);
}