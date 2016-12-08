// ---------------------------------------------------------------------
// $Id$
//
//   Random.h (created on Tue Nov 08 01:08:35 BRT 2005)
// 
//   Genetic Programming in OpenCL (GPOCL)
//
//   Copyright (C) 2005-2008 Douglas Adriano Augusto
// 
// This file is part of gpocl.
// 
// gpocl is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 3 of the License, or (at your option) any later
// version.
// 
// gpocl is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU General Public License along with
// gpocl; if not, see <http://www.gnu.org/licenses/>.
//
// ---------------------------------------------------------------------

#ifndef random_h
#define random_h

#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>

typedef unsigned long uint64_t;
typedef long int64_t;
// FIXME: use the line below instead (which requires a C++11 compiler)
//#include <cstdint>

// TODO: Make Random thread-safe (like XorShift128Plus)
// ---------------------------------------------------------------------
/**
 * Simple random number generator (based on standard rand() function).
 */
class Random {
public:
   /** Sets the random seed. (0 = "random") */
   static uint64_t Seed( uint64_t seed = 0L ) 
   { 
      srand( seed = ( seed == 0L ) ? time( NULL ) : seed );
      return seed;
   }

   /** Uniform random [0:rand_max] */
   static uint64_t Int() { return rand(); }

   /** Uniform random (integer) [0:n) -- greater than or equal 0 but less than n */
   static uint64_t Int( uint64_t n )
   {
      return static_cast<uint64_t>( double( Int() ) * n  / (rand_max + 1.0) );
   }    

   /** Uniform random (integer) [a:b] */
   static int64_t Int( int64_t a, int64_t b )
   {
      return a + static_cast<int64_t>( Int() * (b - a + 1.0) / (rand_max + 1.0) );
   }    

   /** Uniform random (real) [a:b) -- includes 'a' but not 'b' */
   static double Real()
   {
      return double( Int() ) / (double( rand_max ) + 1.0);
   }

   /** Uniform random (real) [a:b] -- includes 'a' and 'b' */
   static double Real( double a, double b )
   {
      return a + Int() * (b - a) / double( rand_max );
   }

   /* Non Uniform Random Numbers
    *
    * 'weight = 1': uniform distribution between 'a' and 'b' [a,b]
    * 'weight > 1': non uniform distribution towards 'a'
    * 'weight < 1': non uniform distribution towards 'b'
    */

   /** Non-uniform. Integer version [a,b) -- includes 'a' but not 'b' */
   static int64_t NonUniformInt( double weight, int64_t a, int64_t b )
   {
      return static_cast<int64_t>( pow( Real(), weight ) * (b - a) + a );
   }

   /** Non-uniform. Float version [a,b) */
   static double NonUniformReal( double weight, double a = 0.0, double b = 1.0 )
   {
      return pow( Real(), weight ) * (b - a) + a;
   }

   /** Probability ("flip coin"): [0% = 0.0 and 100% = 1.0] */
   static bool Probability( double p )
   {
      if( p <= 0.0 ) return false;
      if( p >= 1.0 ) return true;

      return Real() < p ? true : false;
   }
protected:
   static uint64_t const rand_max = RAND_MAX;
};

// TODO: Avoid code duplication!
////////////////////////////////////////////////////////////////////////////////
/* XorShift128Plus (https://en.wikipedia.org/wiki/Xorshift) */
////////////////////////////////////////////////////////////////////////////////
//class XorShift128Plus: public Random {
class XorShift128Plus {
public:

//uint64_t s[2] = { (uint64_t(rd()) << 32) ^ (rd()), (uint64_t(rd()) << 32) ^ (rd()) };
//double random_number() { return (double) xorshift128plus()/((double)18446744073709551615ULL + 1.0); };
   XorShift128Plus(uint64_t id = 0) {
      // Shift by id so that many created instances (having different id's)
      // will have different (pseudo)-random sequences
      s[0] = 12345 + id;
      s[1] = 67890 + id;
   }

uint64_t Int()
{
   //std::cerr << '.';
   uint64_t x = s[0];
   uint64_t const y = s[1];
   s[0] = y;
   x ^= x << 23; // a
   s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
   return s[1] + y;
}
public:
   /** Sets the random seed. (0 = "random") */
   uint64_t Seed( uint64_t seed = 0L ) 
   {
      seed = ( seed == 0L ) ? time( NULL ) : seed;
      s[0] = (uint64_t(seed) << 32) | (seed & 0xffffffff);
      s[1] = (uint64_t(seed) << 32) | (seed & 0xffffffff);

      return seed;
   }

   uint64_t Int( uint64_t n )
   {
      return static_cast<uint64_t>( double( Int() ) * n  / (rand_max + 1.0) );
   }

   /** Uniform random (integer) [a:b] */
   int64_t Int( int64_t a, int64_t b )
   {
      return a + static_cast<int64_t>( Int() * (b - a + 1.0) / (rand_max + 1.0) );
   }

   /** Uniform random (real) [a:b) -- includes 'a' but not 'b' */
   double Real()
   {
      return double( Int() ) / (double( rand_max ) + 1.0);
   }

   /** Uniform random (real) [a:b] -- includes 'a' and 'b' */
   double Real( double a, double b )
   {
      return a + Int() * (b - a) / double( rand_max );
   }

   /* Non Uniform Random Numbers
    *
    * 'weight = 1': uniform distribution between 'a' and 'b' [a,b]
    * 'weight > 1': non uniform distribution towards 'a'
    * 'weight < 1': non uniform distribution towards 'b'
    */

   /** Non-uniform. Integer version [a,b) -- includes 'a' but not 'b' */
   int64_t NonUniformInt( double weight, int64_t a, int64_t b )
   {
      return static_cast<int64_t>( pow( Real(), weight ) * (b - a) + a );
   }

   /** Non-uniform. Float version [a,b) */
   double NonUniformReal( double weight, double a = 0.0, double b = 1.0 )
   {
      return pow( Real(), weight ) * (b - a) + a;
   }

   /** Probability ("flip coin"): [0% = 0.0 and 100% = 1.0] */
   bool Probability( double p )
   {
      if( p <= 0.0 ) return false;
      if( p >= 1.0 ) return true;

      return Real() < p ? true : false;
   }
protected:
   //static uint64_t const rand_max = std::numeric_limits<uint64_t>::max();
   static uint64_t const rand_max = 18446744073709551615UL;
   uint64_t s[2];
   uint64_t id;
};

// --------------------------------------------------------------------

#endif
