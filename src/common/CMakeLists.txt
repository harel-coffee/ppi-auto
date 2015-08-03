## Find the required POCO C++ libraries (they will be linked further)
FIND_LIBRARY(POCO_FOUNDATION PocoFoundation)
FIND_LIBRARY(POCO_NET PocoNet)

# Create a library called 'common'
ADD_LIBRARY( common common.cc )
