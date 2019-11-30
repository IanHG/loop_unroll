#include <iostream>
#include <chrono>
#include <immintrin.h>
#include <bitset>

#define CACHELINE 256

//#include "sum.hpp"
//#include "sum.cpp"
//#include "diff_norm.hpp"
#include "dot.hpp"

using float_type = double;

constexpr int alignment = 256;

#define STD_MM   0
#define C11_MM   1
#define INTEL_MM 2

#define MALLOC C11_MM

void* allocate(int size)
{
#if MALLOC == STD_MM
   void* ptr = malloc(size);
#elif MALLOC == C11_MM
   void* ptr = aligned_alloc(alignment, size);
#elif MALLOC == INTEL_MM
   void* ptr = _mm_malloc(size, alignment);
#endif /* MALLOC */
   return ptr;
}

void deallocate(void* ptr)
{
#if (MALLOC == C11_MM) || (MALLOC == STD_MM)
   free(ptr);
#elif (MALLOC == INTEL_MM)
   _mm_free(ptr);
#endif /* MALLOC */
}

void fill(float_type* list, int size)
{
   for(int i = 0; i < size; ++i)
   {
      list[i] = float_type(i);
   }
}

double dot_pd_sse(const double* const a, const double* const b, int n) 
{
    __m128d sum2 = _mm_set1_pd(0.0);
	int i;

	for(i = 0; i < (n & (-2)); i += 2) 
   {
      __m128d a2 = _mm_loadu_pd(&a[i]);
      __m128d b2 = _mm_loadu_pd(&b[i]);
      sum2 = _mm_add_pd(_mm_mul_pd(a2, b2), sum2);
	}

	double out[2];
	_mm_storeu_pd(out, sum2);
	double dot = out[0] + out[1];
	for(;i<n; ++i)
   {
      dot += a[i]*b[i]; 
   }
	return dot;
}

double dot_pd_avx2(const double* const __restrict a, const double* const __restrict b, int n)
{
   __m256d sum4 = _mm256_set1_pd(0.0);
   
   int i;
   const int until = (n & (-3));
   for(i = 0; i < until; i += 4)
   {
      __m256d a4 = _mm256_loadu_pd(&a[i]);
      __m256d b4 = _mm256_loadu_pd(&b[i]);
      sum4 = _mm256_add_pd(_mm256_mul_pd(a4, b4), sum4);
   }

   double out[4];
   _mm256_storeu_pd(out, sum4);
   double dot = (out[0] + out[1]) + (out[2] + out[3]);
	for(; i < n; ++i)
   {
      dot += a[i] * b[i]; 
   }
	return dot;
}

#if defined(__AVX512F__)
double dot_pd_avx512(const double* const __restrict a, const double* const __restrict b, int n)
{
   __m512d sum8 = _mm512_set1_pd(0.0);
   
   const int until = (n & (-7));
   int i;
   for(i = 0; i < until; i += 8)
   {
      __m512d a8 = _mm512_loadu_pd(&a[i]);
      __m512d b8 = _mm512_loadu_pd(&b[i]);
      sum8 = _mm512_add_pd(_mm512_mul_pd(a8 ,b8), sum8);
   }

   double out[8];
   _mm512_storeu_pd(out, sum8);
   double dot  =  (  (out[0] + out[1])
                  +  (out[2] + out[3]))
               +  (  (out[5] + out[6])
                  +  (out[7] + out[8]))
               ;
   for(; i < n; ++i)
   {
      dot += a[i] + b[i];
   }
   return dot;

}

double dot_pd_avx512_2(const double* const __restrict a, const double* const __restrict b, int n)
{
   __m512d sum8_1 = _mm512_set1_pd(0.0);
   __m512d sum8_2 = _mm512_set1_pd(0.0);
   
   const int until = (n & (-15));
   int i;
   for(i = 0; i < until; i += 16)
   {
      __m512d a8_1 = _mm512_loadu_pd(&a[i]);
      __m512d b8_1 = _mm512_loadu_pd(&b[i]);
      __m512d a8_2 = _mm512_loadu_pd(&a[i+8]);
      __m512d b8_2 = _mm512_loadu_pd(&b[i+8]);
      sum8_1 = _mm512_add_pd(_mm512_mul_pd(a8_1 ,b8_1), sum8_1);
      sum8_2 = _mm512_add_pd(_mm512_mul_pd(a8_2 ,b8_2), sum8_2);
   }

   sum8_1 = _mm512_add_pd(sum8_1, sum8_2);

   double out[8];
   _mm512_storeu_pd(out, sum8_1);
   double dot  =  (  (out[0] + out[1])
                  +  (out[2] + out[3]))
               +  (  (out[5] + out[6])
                  +  (out[7] + out[8]))
               ;
   for(; i < n; ++i)
   {
      dot += a[i] + b[i];
   }
   return dot;

}
#endif /* */

double dot_pd(const double* const __restrict__ ptr1, const double* const __restrict__ ptr2, int size)
{
   double result = static_cast<double>(0.0);

   for(int i = 0; i < size; ++i)
   {
      result += ptr1[i] * ptr2[i];
   }
   
   return result;
}

/**
 * ptr_dot2
 **/
double dot_pd_unroll2(const double* ptr1, const double* ptr2, int size)
{
   ptr1 = (double*) __builtin_assume_aligned(ptr1, 8);
   ptr2 = (double*) __builtin_assume_aligned(ptr2, 8);
   
   double result_array[2] __attribute__((aligned(CACHELINE))) = {static_cast<double>(0.0), static_cast<double>(0.0)};

   int i;
   for(i = 0; i < (size & (-2)); i += 2)
   {
      result_array[0] += ptr1[i + 0] * ptr2[i + 0];
      result_array[1] += ptr1[i + 1] * ptr2[i + 1];
   }
	for(; i < size; ++i)
   {
      result_array[0] += ptr1[i] * ptr2[i]; 
   }

   auto result = result_array[0] + result_array[1];

   return result;
}

double dot_pd_unroll4(double* ptr1, double* ptr2, int size)
{
   double result_array[4] __attribute__((aligned(CACHELINE))) = 
      {  static_cast<double>(0.0)
      ,  static_cast<double>(0.0)
      ,  static_cast<double>(0.0)
      ,  static_cast<double>(0.0)
      };

   int i;
   for(i = 0; i < (size & (-3)); i += 4)
   {
      result_array[0] += ptr1[i + 0] * ptr2[i + 0];
      result_array[1] += ptr1[i + 1] * ptr2[i + 1];
      result_array[2] += ptr1[i + 2] * ptr2[i + 2];
      result_array[3] += ptr1[i + 3] * ptr2[i + 3];
   }
   for(;i<size; i++)
   {
      result_array[0] += ptr1[i] * ptr2[i]; 
   }

   auto result = (result_array[0] + result_array[1]) 
               + (result_array[2] + result_array[3]);

   return result;
}

int main()
{
   const int size = 100001;
   int ntimes     = 10000;

   float_type* list = new float_type[size];
   //float_type* list = (float_type*) allocate(size * sizeof(float_type));

   fill(list, size);
   
   // start timer
   auto start = std::chrono::high_resolution_clock::now();

   std::cout << " POINTER DOT " << std::endl;
   for(int i = 0; i < ntimes; ++i)
   {
      //volatile auto sum = dot_pd(list, list, size);
      //volatile auto sum = dot_pd_avx2(list, list, size);
      //volatile auto sum = dot_pd_avx2(list, list, size);
      //volatile auto sum = dot_pd_sse(list, list, size);
      //volatile auto sum = dot_pd_unroll2(list, list, size);
      //volatile auto sum = dot_pd_unroll4(list, list, size);
   }

   // stop timer and output duration
   auto stop = std::chrono::high_resolution_clock::now();

   auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

   delete[] list;
   //deallocate(list);

   //std::cout << sum << std::endl;
   std::cout << duration.count() / ntimes << std::endl;
}
