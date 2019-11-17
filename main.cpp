#include <iostream>
#include <chrono>

#define CACHELINE 256

//#include "sum.hpp"
//#include "sum.cpp"
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

int main()
{
   const int size = 100000;

   float_type* list = new float_type[size];
   //float_type* list = (float_type*) allocate(size * sizeof(float_type));

   fill(list, size);
   
   // start timer
   auto start = std::chrono::high_resolution_clock::now();

   //// Do sum
   //register double sum = double(0.0);
   //for(int i = 0; i < size; ++i)
   //{
   //   sum += list[i];
   //}
   auto sum = ptr_dot(list, list, size);

   // stop timer and output duration
   auto stop = std::chrono::high_resolution_clock::now();

   auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

   delete[] list;
   //deallocate(list);

   std::cout << sum << std::endl;
   std::cout << duration.count() << std::endl;
}
