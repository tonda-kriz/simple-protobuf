#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include <proto/addressbook.pb.h>
#include <vector>

int main()
{
    std::vector<std::byte> buffer;
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

    ankerl::nanobench::Bench().minEpochIterations(100000).run("pb-serialize",
                                                              [&]
                                                              {
                                                                  auto size =
                                                                      spb::pb::serialize(book, buffer);
                                                                  ankerl::nanobench::doNotOptimizeAway(size);
                                                              });

    ankerl::nanobench::Bench().minEpochIterations(100000).run(
        "pb-deserialize",
        [&]
        {
            const auto book = spb::pb::deserialize<benchmark::AddressBook>(buffer);
            ankerl::nanobench::doNotOptimizeAway(book);
        });
}
