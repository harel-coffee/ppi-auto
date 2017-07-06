////////////////////////////////////////////////////////////////////////////////
//  Parallel Program Induction (PPI) is free software: you can redistribute it
//  and/or modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation, either version 3 of the License,
//  or (at your option) any later version.
//
//  PPI is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with PPI.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#ifndef M_PI_F
   #define M_PI_F 3.14159274101257f
#endif
#ifndef M_PI_2_F
   #define M_PI_2_F 1.57079637050629f
#endif
#ifndef M_PI_4_F
   #define M_PI_4_F 0.78539818525314f
#endif
#ifndef M_1_PI_F
   #define M_1_PI_F 0.31830987334251f
#endif
#ifndef M_2_PI_F
   #define M_2_PI_F 0.63661974668503f
#endif
#ifndef M_2_SQRTPI_F
   #define M_2_SQRTPI_F 1.12837922573090f
#endif
#ifndef M_SQRT2_F
   #define M_SQRT2_F 1.41421353816986f
#endif
#ifndef M_SQRT1_2_F
   #define M_SQRT1_2_F 0.70710676908493f
#endif
#ifndef M_E_F
   #define M_E_F 2.71828174591064f
#endif
#ifndef M_LOG2E_F
   #define M_LOG2E_F 1.44269502162933f
#endif
#ifndef M_LOG10E_F
   #define M_LOG10E_F 0.43429449200630f
#endif
#ifndef M_LN2_F
   #define M_LN2_F 0.69314718246460f
#endif
#ifndef M_LN10_F
   #define M_LN10_F 2.30258512496948f
#endif

float SIN(float x) {
#ifdef NATIVE
   return native_sin(x);
#else
   return sin(x);
#endif
}

float COS(float x) {
#ifdef NATIVE
   return native_cos(x);
#else
   return cos(x);
#endif
}

float EXP(float x) {
#ifdef NATIVE
   return native_exp(x);
#else
   return exp(x);
#endif
}

float EXP10(float x) {
#ifdef NATIVE
   return native_exp10(x);
#else
   return exp10(x);
#endif
}

float EXP2(float x) {
#ifdef NATIVE
   return native_exp2(x);
#else
   return exp2(x);
#endif
}

float DIV(float x, float y) {
#ifdef PROTECTED
   #ifdef NATIVE
     return y==0.0f ? 1.0f : native_divide(x, y);
   #else
     return y==0.0f ? 1.0f : x/y;
   #endif
#else
   #ifdef NATIVE
     return native_divide(x, y);
   #else
     return x/y;
   #endif
#endif
}

float SQRT(float x) {
#ifdef PROTECTED
   #ifdef NATIVE
     return x<0.0f ? 1.0f : native_sqrt(x);
   #else
     return x<0.0f ? 1.0f : sqrt(x);
   #endif
#else
   #ifdef NATIVE
     return native_sqrt(x);
   #else
     return sqrt(x);
   #endif
#endif
}

float LOG(float x) {
#ifdef PROTECTED
   #ifdef NATIVE
     return x<1.0f ? 1.0f : native_log(x);
   #else
     return x<1.0f ? 1.0f : log(x);
   #endif
#else
   #ifdef NATIVE
     return native_log(x);
   #else
     return log(x);
   #endif
#endif
}

float LOG10(float x) {
#ifdef PROTECTED
   #ifdef NATIVE
     return x<1.0f ? 1.0f : native_log10(x);
   #else
     return x<1.0f ? 1.0f : log10(x);
   #endif
#else
   #ifdef NATIVE
     return native_log10(x);
   #else
     return log10(x);
   #endif
#endif
}

float LOG2(float x) {
#ifdef PROTECTED
   #ifdef NATIVE
     return x<1.0f ? 1.0f : native_log2(x);
   #else
     return x<1.0f ? 1.0f : log2(x);
   #endif
#else
   #ifdef NATIVE
     return native_log2(x);
   #else
     return log2(x);
   #endif
#endif
}

float TAN(float x) {
#ifdef PROTECTED
   #ifdef NATIVE
     return (fabs(x)==M_PI_2_F || fabs(x)==3.0f*M_PI_2_F) ? 1.0f : native_tan(x);
   #else
     return (fabs(x)==M_PI_2_F || fabs(x)==3.0f*M_PI_2_F) ? 1.0f : tan(x);
   #endif
#else
   #ifdef NATIVE
     return native_tan(x);
   #else
     return tan(x);
   #endif
#endif
}

#endif
