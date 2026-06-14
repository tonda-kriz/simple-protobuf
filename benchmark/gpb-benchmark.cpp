#define ANKERL_NANOBENCH_IMPLEMENT
#include "google/protobuf/util/json_util.h"
#include "gpb-common.h"
#include "nanobench.h"
#include <vector>

int main()
{
    std::string buffer;
    buffer.reserve(1024);

    gpb::benchmark::AddressBook book;
    init_message(book);

    ankerl::nanobench::Bench().minEpochIterations(100000).run("gpb-serialize",
                                                              [&]
                                                              {
                                                                  const auto res =
                                                                      book.SerializeToString(&buffer);
                                                                  ankerl::nanobench::doNotOptimizeAway(res);
                                                              });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("gpb-init-serialize",
                                                              [&buffer]
                                                              {
                                                                  gpb::benchmark::AddressBook book;
                                                                  init_message(book);
                                                                  const auto res =
                                                                      book.SerializeToString(&buffer);
                                                                  ankerl::nanobench::doNotOptimizeAway(res);
                                                              });

    ankerl::nanobench::Bench().minEpochIterations(100000).run("gpb-deserialize",
                                                              [&buffer]
                                                              {
                                                                  gpb::benchmark::AddressBook book;
                                                                  const auto res =
                                                                      book.ParseFromString(buffer);
                                                                  ankerl::nanobench::doNotOptimizeAway(res);
                                                              });

    ankerl::nanobench::Bench().minEpochIterations(10000).run(
        "json-serialize",
        [&]
        {
            buffer.clear();
            const auto res = google::protobuf::util::MessageToJsonString(book, &buffer);
            ankerl::nanobench::doNotOptimizeAway(res);
        });

    ankerl::nanobench::Bench().minEpochIterations(10000).run(
        "json-init-serialize",
        [&buffer]
        {
            gpb::benchmark::AddressBook book;
            init_message(book);
            buffer.clear();
            const auto res = google::protobuf::util::MessageToJsonString(book, &buffer);
            ankerl::nanobench::doNotOptimizeAway(res);
        });

    ankerl::nanobench::Bench().minEpochIterations(10000).run(
        "json-deserialize",
        [&]
        {
            gpb::benchmark::AddressBook book;
            const auto res = google::protobuf::util::JsonStringToMessage(buffer, &book);
            ankerl::nanobench::doNotOptimizeAway(book);
        });
}
