cmake_minimum_required (VERSION 2.6) #specify the required version of cmake
project (IIO)                        #give a name to the project/solution

# ADD LOCAL PATH TO SEARCH MODULES
list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_PREFIX}")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})



# SETUP A MAKE CLEAN TO CLEAN EVERYTHING
#
#
set_directory_properties (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES 
   "CMakeCache.txt;CMakeFiles;Makefile;cmake_install.cmake;install_manifest.txt;iio;iion;vflip")


# LIBRARIES
#
#

set (LIBS "")

# IF AVAILABLE USE OPENEXR
FIND_PACKAGE(OPENEXR) 
# OPENEXR_FOUND = OpenEXR found. 
# OPENEXR_INCLUDE_PATHS = OpenEXR include directories.
# OPENEXR_LIBRARIES = libraries that are needed to use OpenEXR.
IF (OPENEXR_FOUND) 
   include_directories(${OPENEXR_INCLUDE_PATHS})
   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DI_CAN_HAS_LIBEXR")
   SET( LIBS ${LIBS} ${OPENEXR_LIBRARIES})
ENDIF()

# OTHER LIBRARIES
FIND_PACKAGE(TIFF)
IF(TIFF_FOUND)
   include_directories(${TIFF_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${TIFF_LIBRARIES})
ENDIF()

FIND_PACKAGE(JPEG)
if(JPEG_FOUND)
   include_directories(${JPEG_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${JPEG_LIBRARIES})
ENDIF()

FIND_PACKAGE(PNG)
IF(PNG_FOUND)
   include_directories(${PNG_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${PNG_LIBRARIES})
ENDIF()



# DETECT LINUX
#
#
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   # Linux specific code flags
   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DI_CAN_HAS_LINUX")
ENDIF()


############################################################################



# IIOLIB
#
#
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=c99 -funroll-loops -Wno-unused -DNDEBUG")
message( "FLAGS: ${CMAKE_C_FLAGS}")
set(IIO_LIB_SRCS iio.h iio.c)       #Define a variable that contains files of the lib
add_library(IIOLIB ${IIO_LIB_SRCS}) #Define that those files create the lib FooLib

# IIO command lines
#
#
add_executable (iion iio_test_named.c)
target_link_libraries(iion IIOLIB ${LIBS})
add_executable (iio iio_test.c)
target_link_libraries(iio IIOLIB ${LIBS})


# X11
#
#
FIND_PACKAGE(X11 REQUIRED)
IF(X11_FOUND)
   include_directories(${X11_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${X11_LIBRARIES})
ENDIF()


# VFLIP
#
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -funroll-loops -Wno-unused")
add_executable (vflip v.cpp)
target_link_libraries(vflip IIOLIB ${LIBS} z m dl pthread)



# OPENGL and GLUT
#
#
FIND_PACKAGE(OPENGL REQUIRED)
IF(OPENGL_FOUND)
   include_directories(${OPENGL_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${OPENGL_LIBRARIES})
ENDIF()
FIND_PACKAGE(GLUT REQUIRED)
IF(GLUT_FOUND)
   include_directories(${GLUT_INCLUDE_DIR})
   SET( LIBS ${LIBS} ${GLUT_LIBRARIES})
ENDIF()

# VFLIPGL
#
#
add_executable (ff vGL.c)
target_link_libraries(ff IIOLIB ${LIBS})


# INSTALL
#
#
#set(CMAKE_INSTALL_PREFIX "$ENV{HOME}")
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
     SET(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "IIO install prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
message("Install destination ${CMAKE_INSTALL_PREFIX}/bin") 
INSTALL_TARGETS(${CMAKE_INSTALL_PREFIX}/bin vflip iio iion)
