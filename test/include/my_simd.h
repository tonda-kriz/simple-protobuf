#pragma once

#include <version>

#if __cpp_lib_simd >= 202411L
#include <simd>
#else
#include <experimental/simd>
namespace std
{
template < class Tp, int Np >
using fixed_size_simd = experimental::fixed_size_simd< Tp, Np >;
}
#endif
