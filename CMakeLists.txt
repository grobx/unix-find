cmake_minimum_required (VERSION 3.12)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("find" LANGUAGES C CXX)

add_executable (find "find.cpp")
set_property(TARGET find PROPERTY CXX_STANDARD 23)
target_compile_options(find PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-exceptions>)

add_library (trace SHARED "trace.cpp")
target_compile_options(trace PRIVATE -shared -fPIC)

#target_link_libraries(find PRIVATE trace)
