#pragma once

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