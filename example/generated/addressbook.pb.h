#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <spb/json.hpp>
#include <spb/pb.hpp>
#include <vector>

namespace tutorial
{
struct Person
{
    enum class PhoneType : int32_t
    {
        PHONE_TYPE_UNSPECIFIED = 0,
        PHONE_TYPE_MOBILE      = 1,
        PHONE_TYPE_HOME        = 2,
        PHONE_TYPE_WORK        = 3,
    };
    struct PhoneNumber
    {
        std::optional< std::string > number;
        std::optional< PhoneType > type = PhoneType::PHONE_TYPE_HOME;
    };
    std::optional< std::string > name;
    std::optional< int32_t > id;
    std::optional< std::string > email;
    std::vector< PhoneNumber > phones;
};
struct AddressBook
{
    std::vector< Person > people;
};
}// namespace tutorial

namespace spb::json::detail
{
struct ostream;
struct istream;

void serialize_value( ostream & stream, const ::tutorial::Person & value );
void deserialize_value( istream & stream, ::tutorial::Person & value );

void serialize_value( ostream & stream, const ::tutorial::AddressBook & value );
void deserialize_value( istream & stream, ::tutorial::AddressBook & value );

void serialize_value( ostream & stream, const ::tutorial::Person::PhoneNumber & value );
void deserialize_value( istream & stream, ::tutorial::Person::PhoneNumber & value );

void serialize_value( ostream & stream, const ::tutorial::Person::PhoneType & value );
void deserialize_value( istream & stream, ::tutorial::Person::PhoneType & value );

}// namespace spb::json::detail
namespace spb::pb::detail
{
struct ostream;
struct istream;

void serialize( ostream & stream, const ::tutorial::Person & value );
void deserialize_value( istream & stream, ::tutorial::Person & value, tag_type tag );

void serialize( ostream & stream, const ::tutorial::AddressBook & value );
void deserialize_value( istream & stream, ::tutorial::AddressBook & value, tag_type tag );

void serialize( ostream & stream, const ::tutorial::Person::PhoneNumber & value );
void deserialize_value( istream & stream, ::tutorial::Person::PhoneNumber & value, tag_type tag );

}// namespace spb::pb::detail
