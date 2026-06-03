#include <proto/addressbook.pb.h>
#include <vector>

int main()
{
    std::string buffer;
    buffer.reserve(256);
    const auto book = benchmark::AddressBook{
        .people = {
            {.name = "john",
             .id = 587965,
             .email = "john@carter.com",
             .phones = {{.number = "777888999", .type = benchmark::Person::PhoneType::PHONE_TYPE_MOBILE},
                        {.number = "456895251", .type = benchmark::Person::PhoneType::PHONE_TYPE_HOME}}},
            {.name = "hanz",
             .id = 89,
             .email = "hanz@obergartenmeister.at",
             .phones = {{.number = "5689571", .type = benchmark::Person::PhoneType::PHONE_TYPE_MOBILE},
                        {.number = "6589652", .type = benchmark::Person::PhoneType::PHONE_TYPE_HOME}}},
        }};

    const auto size = spb::json::serialize(book, buffer);
    return size > 0 ? 0 : 1;
}