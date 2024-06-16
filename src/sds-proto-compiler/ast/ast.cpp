/***************************************************************************\
* Name        : ast render                                                  *
* Description : resolve types in ast tree                                   *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#include "ast.h"
#include "io/file.h"
#include "proto-field.h"
#include "proto-file.h"
#include "proto-import.h"
#include "proto-message.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <ranges>
#include <sds/char_stream.h>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
[[nodiscard]] auto is_imported( std::string_view type, const proto_files & imports ) -> bool
{
    return std::ranges::any_of( imports, [ = ]( const auto & import ) -> bool
                                { return type == import.package.name; } );
}

[[nodiscard]] auto is_enum( std::string_view type, const proto_enums & enums ) -> bool
{
    return std::ranges::any_of( enums, [ = ]( const auto & enum_field ) -> bool
                                { return type == enum_field.name; } );
}

[[nodiscard]] auto is_sub_message( std::string_view type, const proto_messages & messages ) -> bool
{
    return std::ranges::any_of( messages, [ = ]( const auto & message ) -> bool
                                { return type == message.name; } );
}

[[nodiscard]] auto is_resolved_sub_message( std::string_view type, const proto_messages & messages ) -> bool
{
    return std::ranges::any_of( messages, [ = ]( const auto & message ) -> bool
                                { return type == message.name && message.resolved > 0; } );
}

[[nodiscard]] auto is_sub_oneof( std::string_view type, const proto_oneofs & oneofs ) -> bool
{
    return std::ranges::any_of( oneofs, [ = ]( const auto & oneof ) -> bool
                                { return type == oneof.name; } );
}

struct search_state
{
    enum resolve_mode
    {
        //- only check if type is already defined
        dependencies_only,
        //- for optional fields create an std::unique_ptr< T > type and forward declare T
        //- to break cyclic dependency
        optional_pointers,
    };

    resolve_mode mode        = dependencies_only;
    size_t resolved_messages = 0;
    const proto_files & imports;
};

/**
 * @brief search ctx to hold relation 'message -> parent_message'
 *        the relation is not stored in proto_message because its not needed until now
 *        and the messages get sorted (moved) later so the parent pointers will be invalid anyways
 *        the parent relation is used for type dependency check and for proper order of structs definition
 *        in the generated *.pb.h header file, because C++ needs proper order of type dependencies.
 *        The proper order is defined by the value of `.resolved` for every message
 *
 */
struct search_ctx
{
    proto_message & message;
    //- parent message (can be null for top level)
    search_ctx * p_parent;
    search_state & state;
};

/**
 * @brief if the field is self-referencing and can be forward declared
 *        field label must be LABEL_OPTIONAL or LABEL_PTR or LABEL_REPEATED
 *
 * @param field filed
 * @param ctx search context
 * @return true if field can be forward declared
 */
[[nodiscard]] auto is_self( proto_field & field, const search_ctx & ctx ) -> bool
{
    if( field.type == ctx.message.name )
    {
        switch( field.label )
        {
        case proto_field::Label::LABEL_NONE:
            throw std::runtime_error( std::format( "Field '{}' cannot be self-referencing (make it optional)", field.name ) );
        case proto_field::Label::LABEL_OPTIONAL:
            field.label = proto_field::Label::LABEL_PTR;
            return true;
        case proto_field::Label::LABEL_REPEATED:
        case proto_field::Label::LABEL_PTR:
            return true;
        }
    }
    return false;
}

/**
 * @brief if this dependency can be forward declared, do it
 *        field label must be LABEL_REPEATED or LABEL_PTR
 *
 * @param field field
 * @param ctx search context
 * @return true if field can be forward declared
 */
[[nodiscard]] auto is_forwarded( proto_field & field, search_ctx & ctx ) -> bool
{
    if( ctx.p_parent == nullptr )
    {
        return false;
    }

    for( auto & message : ctx.p_parent->message.messages )
    {
        if( field.type == message.name )
        {
            switch( field.label )
            {
            case proto_field::Label::LABEL_NONE:
                return false;
            case proto_field::Label::LABEL_OPTIONAL:
                switch( ctx.state.mode )
                {
                case search_state::dependencies_only:
                    return false;

                case search_state::optional_pointers:
                    field.label = proto_field::Label::LABEL_PTR;
                    //
                    //- revert the mode to dependencies_only and try again
                    //- reason is to create as little pointers as possible
                    //
                    ctx.state.mode = search_state::dependencies_only;
                }
                [[fallthrough]];
            case proto_field::Label::LABEL_REPEATED:
            case proto_field::Label::LABEL_PTR:
                ctx.p_parent->message.forwards.insert( message.name );
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief if this dependency references the parent message and can be forward declared
 *        field label must be LABEL_OPTIONAL or LABEL_REPEATED or LABEL_PTR
 *
 * @param field field
 * @param ctx search context
 * @return true if field can be forward declared
 */
[[nodiscard]] auto is_parent( proto_field & field, const search_ctx & ctx ) -> bool
{
    if( ctx.p_parent == nullptr )
    {
        return false;
    }

    if( field.type == ctx.p_parent->message.name )
    {
        switch( field.label )
        {
        case proto_field::Label::LABEL_NONE:
            throw std::runtime_error( std::format( "Field '{}' cannot reference parent (make it optional)", field.name ) );
        case proto_field::Label::LABEL_OPTIONAL:
            field.label = proto_field::Label::LABEL_PTR;
            return true;
        case proto_field::Label::LABEL_REPEATED:
        case proto_field::Label::LABEL_PTR:
            return true;
        }
    }
    return false;
}

[[nodiscard]] auto is_defined_in_parents( std::string_view type, const search_ctx & ctx ) -> bool
{
    if( ctx.p_parent == nullptr )
    {
        return false;
    }

    const auto & message = ctx.p_parent->message;
    if( is_enum( type, message.enums ) ||
        is_resolved_sub_message( type, message.messages ) ||
        is_sub_oneof( type, message.oneofs ) )
    {
        return true;
    }

    return is_defined_in_parents( type, *ctx.p_parent );
}

[[nodiscard]] auto all_types_are_resolved( const proto_messages & messages ) -> bool
{
    return std::ranges::all_of( messages, []( const auto & message )
                                { return message.resolved > 0; } );
}

void mark_message_as_resolved( search_ctx & ctx )
{
    assert( ctx.message.resolved == 0 );

    ctx.message.resolved = ++ctx.state.resolved_messages;
}

void resolve_message_fields( search_ctx & ctx )
{
    if( ctx.message.resolved > 0 )
    {
        return;
    }

    for( auto & field : ctx.message.fields )
    {
        //- check only the first type (before .) and leave the rest for the C++ compiler to check
        //- TODO: check full type name
        const auto type = field.type.substr( 0, field.type.find( '.' ) );

        if( is_scalar_type( field.type ) ||
            is_self( field, ctx ) ||
            is_enum( field.type, ctx.message.enums ) ||
            is_sub_message( type, ctx.message.messages ) ||
            is_sub_oneof( type, ctx.message.oneofs ) ||
            is_parent( field, ctx ) ||
            is_defined_in_parents( type, ctx ) ||
            is_imported( type, ctx.state.imports ) ||
            is_forwarded( field, ctx ) )
        {
            continue;
        }

        //- can't forward declare sub messages
        return;
    }

    for( const auto & map : ctx.message.maps )
    {
        //- check only the first type (before .) and leave the rest for the C++ compiler to check
        const auto type = map.value_type.substr( 0, map.value_type.find( '.' ) );
        if( !is_scalar_type( map.value_type ) &&
            !is_enum( map.value_type, ctx.message.enums ) &&
            !is_sub_message( type, ctx.message.messages ) &&
            !is_defined_in_parents( type, ctx ) &&
            !is_imported( type, ctx.state.imports ) )
        {
            // std::cout << "map" << map.name << " type: " << type << " unknown\n";
            return;
        }
    }

    for( const auto & oneof : ctx.message.oneofs )
    {
        for( const auto & field : oneof.fields )
        {
            //- check only the first type (before .) and leave the rest for the C++ compiler to check
            const auto type = field.type.substr( 0, field.type.find( '.' ) );
            if( !is_scalar_type( field.type ) &&
                !is_enum( field.type, ctx.message.enums ) &&
                !is_sub_message( type, ctx.message.messages ) &&
                !is_defined_in_parents( type, ctx ) &&
                !is_imported( type, ctx.state.imports ) )
            {
                // std::cout << "oneof " << oneof.name << " type: " << type << " unknown\n";
                return;
            }
        }
    }

    if( all_types_are_resolved( ctx.message.messages ) )
    {
        mark_message_as_resolved( ctx );
    }
}

void resolve_message_dependencies( search_ctx & ctx )
{
    if( ctx.message.resolved > 0 )
    {
        return;
    }

    for( auto & message : ctx.message.messages )
    {
        auto sub_ctx = search_ctx{
            .message  = message,
            .p_parent = &ctx,
            .state    = ctx.state,
        };

        resolve_message_dependencies( sub_ctx );
    }
    resolve_message_fields( ctx );
}

void dump_unresolved_messages( const proto_messages & messages, std::string_view parent )
{
    for( const auto & message : messages )
    {
        if( message.resolved == 0 )
        {
            const auto full_name = std::format( "{}.{}", parent, message.name );
            std::cout << "unresolved message " << full_name << '\n';
            dump_unresolved_messages( message.messages, full_name );
        }
    }
}

void sort_messages( proto_messages & messages )
{
    std::sort( messages.begin( ), messages.end( ), []( const auto & a, const auto & b )
               { return a.resolved < b.resolved; } );

    for( auto & message : messages )
    {
        sort_messages( message.messages );
    }
}

}// namespace

[[nodiscard]] auto is_scalar_type( std::string_view type ) -> bool
{
    static constexpr auto std_types = std::array< std::string_view, 15 >{ {
        { "bool" },
        { "bytes" },
        { "double" },
        { "float" },
        { "int32" },
        { "int64" },
        { "uint32" },
        { "uint64" },
        { "sint32" },
        { "sint64" },
        { "fixed32" },
        { "fixed64" },
        { "sfixed32" },
        { "sfixed64" },
        { "string" },
    } };

    return std::find( std_types.begin( ), std_types.end( ), type ) != std_types.end( );
}

void resolve_messages( proto_file & file )
{
    auto state = search_state{
        .mode              = search_state::dependencies_only,
        .resolved_messages = 0,
        .imports           = file.file_imports,
    };

    while( !all_types_are_resolved( file.package.messages ) )
    {
        const auto resolved_messages = state.resolved_messages;

        auto top_level_ctx = search_ctx{
            .message  = file.package,
            .p_parent = nullptr,
            .state    = state,
        };

        resolve_message_dependencies( top_level_ctx );

        if( resolved_messages == state.resolved_messages )
        {
            if( state.mode == search_state::dependencies_only )
            {
                //- no messages were resolved using only dependencies?
                //- try to break cyclic dependency by using forward pointers declaration
                state.mode = search_state::optional_pointers;
                continue;
            }

            dump_unresolved_messages( file.package.messages, "" );
            throw std::runtime_error( "type dependency can't be resolved" );
        }
    }

    sort_messages( file.package.messages );
}
