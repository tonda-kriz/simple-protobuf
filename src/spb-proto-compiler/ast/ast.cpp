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
#include "ast-types.h"
#include "ast-messages-order.h"

void resolve_messages( proto_file & file )
{
    resolve_types( file );
    resolve_messages_order( file );
}

const proto_message* find_message(const proto_message& parent, const std::string_view& proto_name)
{
    if (parent.name.proto_name == proto_name)
    {
        return &parent;
    }
    for (const proto_message& child : parent.messages)
    {
        const proto_message* p = find_message(child, proto_name);
        if (p != nullptr)
        {
            return p;
        }
    }
    return nullptr;
}

const proto_message* find_message(const proto_file& file, const std::string_view& proto_name)
{
    const proto_message* type_message = find_message(file.package, proto_name);
    if (type_message == nullptr)
    {
        for (const auto& f: file.file_imports)
        {
            type_message = find_message(f.package, proto_name);
            if (type_message != nullptr)
            {
                break;
            }
        }
    }
    return type_message;
}
