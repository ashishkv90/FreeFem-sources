IF(OpenGL_FOUND AND GLUT_FOUND)

  EXECUTE_PROCESS(COMMAND "date" OUTPUT_VARIABLE FREEFEM_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
  FILE(WRITE ${CMAKE_SOURCE_DIR}/src/medit/compil.date "\#define COMPIL  \" ${FREEFEM_DATE} (with ff++ ${FREEFEM_VERSION})\"")

  FILE(GLOB_RECURSE CFILES *.c)
  ADD_EXECUTABLE(ffmedit ${CFILES})
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/medit ${CMAKE_SOURCE_DIR}/src/libMesh)
  TARGET_LINK_LIBRARIES(ffmedit libMesh m ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLUT_LIBRARIES} )

  INSTALL(TARGETS ffmedit
          RUNTIME DESTINATION bin) 

ENDIF(OpenGL_FOUND AND GLUT_FOUND)
