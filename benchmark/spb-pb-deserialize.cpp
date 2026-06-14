#include "spb-common.h"
#include <string>

int main()
{
    std::string buffer(127, '-');
    const auto book = spb::pb::deserialize<spb::benchmark::AddressBook>(buffer);
    return book.people.empty() ? 0 : 1;
}