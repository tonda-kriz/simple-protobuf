#pragma once

#if __has_include( <simd>)
#include <simd>
namespace simd_ns = std;
#elif __has_include( <experimental/simd>)
#include <experimental/simd>
namespace simd_ns = std::experimental;
#else
#error "No SIMD implementation found"
#endif

template < class Tp, int Np >
using my_fixed_size_simd = simd_ns::fixed_size_simd< Tp, Np >;
