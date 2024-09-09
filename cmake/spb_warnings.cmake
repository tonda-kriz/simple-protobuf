function(spb_enable_warnings TARGET)
    if(MSVC)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            # clang-cl doesn't support /Zc:preprocessor
            target_compile_options(${TARGET} PRIVATE /W4 /WX /wd4996 /wd4244 /EHs -Wno-missing-field-initializers)
        else()
            # https://developercommunity.visualstudio.com/t/c2017-illegal-escape-sequence-when-using-in-a-raw/919371
            target_compile_options(${TARGET} PRIVATE /W4 /WX /wd4996 /wd4244 /Zc:preprocessor)
        endif()        
        target_compile_definitions(${TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-field-initializers)
    endif()
endfunction(spb_enable_warnings)
