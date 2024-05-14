include(CMakeParseArguments)

if(SDS_PROTO_USE_CLANG_FORMAT)
    find_program(CLANG_FORMAT clang-format)
endif()

function(sds_protobuf_generate SRCS HDRS)
    if (NOT ARGN)
        message(FATAL_ERROR "Error: sds_protobuf_generate() called without any proto files")
        return()
    endif ()
    
    set(${SRCS})
    set(${HDRS})
    
    foreach (FIL ${ARGN})
        get_filename_component(FILE_NAME ${FIL} NAME_WE)
        get_filename_component(FILE_ABS ${FIL} ABSOLUTE)
        
        if(TARGET sds-proto)
            # add directory with generated files to the sds-proto includes
            target_include_directories(sds-proto INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        
        list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.cc")
        list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.h")

        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        if(SDS_PROTO_USE_CLANG_FORMAT AND CLANG_FORMAT)
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.cc"
                       "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.h"
                COMMAND $<TARGET_FILE:sds-protoc> ARGS "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}" ${FILE_ABS}
                COMMAND ${CLANG_FORMAT} ARGS -i "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.cc"
                COMMAND ${CLANG_FORMAT} ARGS -i "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.h"
                DEPENDS sds-protoc ${FILE_ABS}
                COMMENT "Compiling protofile ${FIL}"
                VERBATIM
            )
        else()
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.cc"
                       "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.pb.h"
                COMMAND $<TARGET_FILE:sds-protoc> ARGS "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}" ${FILE_ABS}
                DEPENDS sds-protoc ${FILE_ABS}
                COMMENT "Compiling protofile ${FIL}"
                VERBATIM
            )
        endif()
    endforeach ()

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction(sds_protobuf_generate)
