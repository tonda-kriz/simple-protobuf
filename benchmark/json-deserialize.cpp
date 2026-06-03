#include <proto/addressbook.pb.h>
#include <vector>

int main()
{
    std::string buffer(127, '2');
    const auto book = spb::json::deserialize<benchmark::AddressBook>(buffer);
    return book.people.empty() ? 0 : 1;
}