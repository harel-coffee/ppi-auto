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

/* The macro ERROR(X,Y) will be replaced with the function defined by
 * '-DEF=function' at building time. */
#define ERROR(X,Y) @EF@
/* This will be either '#define REDUCEMAX 1' or nothing, dependeing whether
 * this option was given at building time. */
#cmakedefine REDUCEMAX 1

/* Same for those below...  needed so that the OpenCL compile can see those
 * values. */
#cmakedefine PROTECTED 1
#cmakedefine NATIVE 1
