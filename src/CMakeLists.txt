set(LIKELY_SRC likely.cpp ${CMAKE_SOURCE_DIR}/include/likely.h)
set(LIKELY_LIBS ${REQ_LLVM_LIBRARIES})
set(LIKELY_DEPENDS LLVM)

if(${LIKELY_AUX})
  set(LIKELY_SRC ${LIKELY_SRC} likely_aux.cpp ${CMAKE_SOURCE_DIR}/include/likely_aux.h)
  set(LIKELY_LIBS ${LIKELY_LIBS} opencv_highgui)
  set(LIKELY_DEPENDS ${LIKELY_DEPENDS} OpenCV)
  install(FILES ${CMAKE_SOURCE_DIR}/include/likely_aux.h DESTINATION include)
endif()

if(${LIKELY_SCRIPT})
  # Compile in standard.likely as a string
  set(LIKELY_STANDARD_LIBRARY_CMAKE ${CMAKE_BINARY_DIR}/standard_library.cmake)
  set(LIKELY_STANDARD_LIBRARY_CPP ${CMAKE_BINARY_DIR}/likely_standard_library.cpp)
  file(WRITE ${LIKELY_STANDARD_LIBRARY_CMAKE}
       "set(LIKELY_STANDARD_LIBRARY_CPP ${LIKELY_STANDARD_LIBRARY_CPP})
        set(LIKELY_STANDARD_LIBRARY \"${CMAKE_SOURCE_DIR}/share/likely/standard.likely\")
        if (\${LIKELY_STANDARD_LIBRARY} IS_NEWER_THAN \${LIKELY_STANDARD_LIBRARY_CPP})
        file(WRITE \${LIKELY_STANDARD_LIBRARY_CPP} \"// Probably a bad idea to modify this automatically generated source file!\\n\\n\")
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \"#include \\\"likely.h\\\"\\n\\nconst char *likely_standard_library()\\n{\\n\\treturn\\n\\t\\\"\")
        file(READ \${LIKELY_STANDARD_LIBRARY} STANDARD_LIBRARY_SOURCE)
        string(REPLACE \"\\\\\" \"\\\\\\\\\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        string(REPLACE \"\\\"\" \"\\\\\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        string(REPLACE \"\\n\" \"\\\\n\\\"\\n\\t\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \${STANDARD_LIBRARY_SOURCE})
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \"\\\";\\n}\\n\")
        endif()")
  include(${LIKELY_STANDARD_LIBRARY_CMAKE}) # Run it once
  add_custom_target(likely_standard_library COMMAND ${CMAKE_COMMAND} -P ${LIKELY_STANDARD_LIBRARY_CMAKE})

  set(LIKELY_SRC ${LIKELY_SRC} likely_script.cpp
                               ${LIKELY_STANDARD_LIBRARY_CPP}
                               ${CMAKE_SOURCE_DIR}/include/likely_script.h
                               ${CMAKE_SOURCE_DIR}/share/likely/standard.likely
                               ${CMAKE_SOURCE_DIR}/share/likely/opencv.shim)
  set(LIKELY_LIBS ${LIKELY_LIBS} liblua)
  set(LIKELY_DEPENDS ${LIKELY_DEPENDS} lua likely_standard_library)
  install(FILES ${CMAKE_SOURCE_DIR}/include/likely_script.h DESTINATION include)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely SHARED ${LIKELY_SRC})
add_dependencies(likely ${LIKELY_DEPENDS})
set_target_properties(likely PROPERTIES
                      DEFINE_SYMBOL LIKELY_LIBRARY
                      VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                      SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${LIKELY_LIBS})
install(TARGETS likely
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_SOURCE_DIR}/include/likely.h DESTINATION include)
install(DIRECTORY share DESTINATION .)

if(${LIKELY_SCRIPT})
  add_custom_command(TARGET likely POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E touch ${LIKELY_STANDARD_LIBRARY_CPP})
endif()