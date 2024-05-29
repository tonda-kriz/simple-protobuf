/***************************************************************************\
* Name        : input stream                                                *
* Description : char stream used for parsing json and proto files           *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once
#include <cassert>
#include <cctype>
#include <string_view>

namespace sds
{

struct char_stream
{
private:
    const char * m_begin = nullptr;
    const char * m_end   = nullptr;
    char m_current       = { };

    /**
     * @brief gets the next char from the stream
     *
     * @param skip_white_space if true, skip white spaces
     */
    void update_current( bool skip_white_space ) noexcept
    {
        while( m_begin < m_end )
        {
            m_current = *m_begin;
            if( !skip_white_space || isspace( m_current ) == 0 )
            {
                return;
            }
            m_begin += 1;
        }
        m_current = { };
    }

public:
    explicit char_stream( std::string_view content ) noexcept
        : m_begin( content.data( ) )
        , m_end( m_begin + content.size( ) )
    {
        update_current( true );
    }

    [[nodiscard]] auto begin( ) const noexcept -> const char *
    {
        return m_begin;
    }

    [[nodiscard]] auto end( ) const noexcept -> const char *
    {
        return m_end;
    }

    [[nodiscard]] auto empty( ) const noexcept -> bool
    {
        return m_end <= m_begin;
    }

    [[nodiscard]] auto current_char( ) const noexcept -> char
    {
        return m_current;
    }

    /**
     * @brief consumes `current char` if its equal to c
     *
     * @param c consumed char
     * @return true if char was consumed
     */
    [[nodiscard]] auto consume( char c ) noexcept -> bool
    {
        if( auto current = current_char( ); current == c )
        {
            consume_current_char( true );
            return true;
        }
        return false;
    }

    /**
     * @brief consumes an `token`
     *
     * @param token consumed `token` (whole word)
     * @return true if `token` was consumed
     */
    auto consume( std::string_view token ) noexcept -> bool
    {
        const auto state = *this;

        if( content( ).starts_with( token ) )
        {
            m_begin += token.size( );
            update_current( false );
            auto next = current_char( );
            if( isspace( next ) || !isalnum( next ) )
            {
                update_current( true );
                return true;
            }
            *this = state;
        }
        return false;
    }

    void consume_current_char( bool skip_white_space ) noexcept
    {
        if( begin( ) < end( ) )
        {
            m_begin += 1;
            update_current( skip_white_space );
        }
    }

    /**
     * @brief trim current spaces
     *
     */
    void consume_space( ) noexcept
    {
        update_current( true );
    }

    void skip_to( const char * ptr ) noexcept
    {
        assert( ptr >= begin( ) && ptr <= end( ) );
        m_begin = ptr;
        update_current( true );
    }

    [[nodiscard]] auto content( ) const noexcept -> std::string_view
    {
        return { begin( ), size_t( end( ) - begin( ) ) };
    }
};
}// namespace sds
