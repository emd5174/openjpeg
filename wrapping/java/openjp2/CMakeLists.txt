#wrapping/java/openjp2

# First thing define the common source:
set(openjpegjni_SRCS
  JavaOpenJPEGDecoder.c
  JavaOpenJPEGEncoder.c
  ${OPENJPEG_SOURCE_DIR}/src/bin/common/color.c
  ${OPENJPEG_SOURCE_DIR}/src/bin/common/opj_getopt.c
  ${OPENJPEG_SOURCE_DIR}/src/bin/jp2/convert.c
  )

# JNI binding:
find_package(JNI REQUIRED)
include_directories(
  ${JNI_INCLUDE_DIRS}
  ${OPENJPEG_SOURCE_DIR}/src/lib/openjp2 # openjpeg.h
  ${OPENJPEG_BINARY_DIR}/src/lib/openjp2 # opj_config.h,opj_config_private.h
  ${OPENJPEG_SOURCE_DIR}/src/bin/common  # color.h
  ${OPENJPEG_BINARY_DIR}/src/bin/common  # opj_apps_config.h
  ${OPENJPEG_SOURCE_DIR}/src/bin/jp2     # convert.h
  )
if(BUILD_THIRDPARTY)
 include_directories(
  ${OPENJPEG_SOURCE_DIR}/thirdparty/include
  ${OPENJPEG_SOURCE_DIR}/thirdparty/liblcms2/include
  ${OPENJPEG_SOURCE_DIR}/thirdparty/libpng
  ${OPENJPEG_SOURCE_DIR}/thirdparty/libtiff
  ${OPENJPEG_BINARY_DIR}/thirdparty/libtiff
 )
endif()
#
add_library(openjpegjni MODULE
  ${openjpegjni_SRCS}
  )
# Java module should not have a SONAME:
set_property(TARGET openjpegjni PROPERTY NO_SONAME 1)

TARGET_LINK_LIBRARIES(openjpegjni ${OPENJPEG_LIBRARY_NAME})

target_link_libraries(openjpegjni
  ${PNG_LIBNAME} ${TIFF_LIBNAME} ${LCMS_LIBNAME} ${Z_LIBNAME}
)
if(UNIX)
  target_link_libraries(openjpegjni m)
endif()

install(TARGETS openjpegjni
  EXPORT OpenJPEGTargets
  LIBRARY DESTINATION ${OPENJPEG_INSTALL_JNI_DIR} COMPONENT Libraries
)

# build jar:
find_package(Java 1.5 REQUIRED) # javac, jar

# build dep list:
file(GLOB java_srcs "java-sources/org/openJpeg/*.java")

# make sure target javac dir exists:
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/classes)
# Build java
add_custom_command(
  OUTPUT ${LIBRARY_OUTPUT_PATH}/openjpeg.jar
  COMMAND ${Java_JAVAC_EXECUTABLE} -sourcepath  "${CMAKE_CURRENT_SOURCE_DIR}/java-sources"
  ${java_srcs} -d ${CMAKE_CURRENT_BINARY_DIR}/classes
  COMMAND ${Java_JAR_EXECUTABLE} cvf ${LIBRARY_OUTPUT_PATH}/openjpeg.jar org
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/classes
  DEPENDS ${java_srcs}
  COMMENT "javac *.java; jar cvf -> openjpeg.jar"
)

# name the target
add_custom_target(OpenJPEGJavaJar ALL
  DEPENDS ${LIBRARY_OUTPUT_PATH}/openjpeg.jar
  COMMENT "building openjpeg.jar"
)

install(FILES ${LIBRARY_OUTPUT_PATH}/openjpeg.jar
  DESTINATION ${OPENJPEG_INSTALL_SHARE_DIR} COMPONENT JavaModule
)
