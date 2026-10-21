set(SOURCES byteio.cpp
    charset.cpp
    convert.cpp
    data.cpp
    error.cpp
    inputproc.cpp
    mactab.cpp
    pattern.cpp
    ukengine.cpp
    usrkeymap.cpp
    unikey.cpp
)

add_library(ukengine STATIC ${SOURCES})
set_target_properties(ukengine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(ukengine PRIVATE -funsigned-char -w)
target_include_directories(ukengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
