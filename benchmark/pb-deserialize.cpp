#include <proto/addressbook.pb.h>
#include <vector>

int main()
{
    std::vector<std::byte> buffer(127, std::byte(2));
    const auto book = spb::pb::deserialize<benchmark::AddressBook>(buffer);
    return book.people.empty() ? 0 : 1;
}