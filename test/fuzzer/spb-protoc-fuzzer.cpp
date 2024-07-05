#include <cstdint>
#include <cstdlib>
#include <string_view>

#include <parser/parser.h>

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * Data, size_t Size )
{
    auto file = proto_file{ };
    try
    {
        parse_proto_file_content( std::string_view{ reinterpret_cast< const char * >( Data ), Size }, file );
    }
    catch( ... )
    {
    }
    return 0;
}