#pragma once

#if __has_include( <simd>)
#include <simd>
template < class Tp, int Np >
using my_fixed_size_simd = std::fixed_size_simd< Tp, Np >;
#else
//- no simd, use fixed-size-array instead to satisfy the unit tests
#include <array>
template < class Tp, int Np >
using my_fixed_size_simd = std::array< Tp, Np >;
#endif
