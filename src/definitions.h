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
