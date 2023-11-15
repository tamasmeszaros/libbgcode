set(Core_DOWNSTREAM_DEPS "")

# Core component
add_library(${_libname}_core
   core.hpp
   core.cpp
   bgcode.cpp
   bgcode_impl.hpp
   ${PROJECT_BINARY_DIR}/version.rc
   bgcode.h
   bgcode_cxx_traits.hpp
   bgcode_defs.h
   block_writer.cpp
   block_writer.h
   capi_adaptor.hpp
   cfile_stream.h
   cfile_stream.cpp
   cfile_stream_impl.hpp
   null_stream.h
   null_stream.cpp
   # Add more source files here if needed
)

if (${PROJECT_NAME}_BUILD_SANITIZERS)
    if (CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(${_libname}_core PUBLIC
            -g
            -Wstrict-aliasing
            -fsanitize=address
        )
        target_link_options(${_libname}_core PUBLIC -fsanitize=address)
    elseif (MSVC)
        target_compile_options(${_libname}_core PUBLIC /fsanitize=address /Zi)
        target_link_options(${_libname}_core PUBLIC /DEBUG)
    endif ()
endif ()

target_compile_definitions(${_libname}_core PRIVATE LibBGCode_VERSION=R"\(${LibBGCode_VERSION}\)")

generate_export_header(${_libname}_core
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/core/export.h
)

target_include_directories(${_libname}_core
   PUBLIC
       $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${_srcloc}>
       $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
       $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
       $<INSTALL_INTERFACE:include>
)

add_library(${namespace}${_libname}_core ALIAS ${_libname}_core)

# Set the version of the project
# target_sources(Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
# target_compile_definitions(Core PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR})

set(Core_DOWNSTREAM_DEPS ${Core_DOWNSTREAM_DEPS} PARENT_SCOPE)

install(FILES bgcode.h DESTINATION include/${PROJECT_NAME}/core)
install(FILES bgcode_defs.h DESTINATION include/${PROJECT_NAME}/core)
install(FILES block_writer.h DESTINATION include/${PROJECT_NAME}/core)
install(FILES cfile_stream.h DESTINATION include/${PROJECT_NAME}/core)
install(FILES null_stream.h DESTINATION include/${PROJECT_NAME}/core)
#install(FILES core.hpp DESTINATION include/${PROJECT_NAME}/core)
