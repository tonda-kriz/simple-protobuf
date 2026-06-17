#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <etl/string.h>
#include <etl/vector.h>
#include <memory>
#include <optional>
#include <spb/json.hpp>
#include <spb/pb.hpp>
#include <string>
#include <vector>

namespace ETL::Example
{
struct DeviceStatus
{
    uint32_t device_id : 31;
    uint32_t is_online : 1;
    uint32_t last_heartbeat;
    std::array<char, 8> firmware_version;
    etl::string<16> name;
};
struct Command
{
    enum class CMD : uint8_t
    {
        CMD_RST = 1,
        CMD_READ = 2,
        CMD_WRITE = 3,
        CMD_TST = 4,
    };
    // CMD
    uint8_t command_id : 4;
    // argument for the command
    uint8_t arg : 2;
    // input flag
    uint8_t in_flag : 1;
    // output flag
    uint8_t out_flag : 1;
};
struct CommandQueue
{
    etl::vector<Command, 16> commands;
    etl::vector<DeviceStatus, 16> statuses;
};
} // namespace ETL::Example

namespace spb::pb
{

/**
 * @brief serialize message via writer
 *
 * @param[in] message to be serialized
 * @param[in] on_write function for handling the writes
 * @param[in] options
 * @return serialized size in bytes
 * @throws exceptions only from `on_write`
 */
auto serialize(const auto &message, spb::io::writer on_write, const serialize_options &options) -> size_t;

/**
 * @brief return protobuf serialized size in bytes
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @return serialized size in bytes
 */
auto serialize_size(const auto &message, const serialize_options &options) -> size_t;

/**
 * @brief serialize message into a buffer
 *        Warning: User is responsible for allocating buffers size (use `auto buffer_size =
 * serialize_size(message)`)
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @return serialized size in bytes
 */
auto serialize(const auto &message, void *buffer, const serialize_options &options) -> size_t;

/**
 * @brief serialize message into protobuf
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @param[out] result serialized message
 * @return serialized size in bytes
 * @throws std::runtime_error on error
 * @example `auto serialized = std::vector<std::byte>();`
 *          `spb::pb::serialize(message, serialized);`
 */
template <spb::resizable_container Container>
auto serialize(const auto &message, Container &result, const serialize_options &options) -> size_t;

/**
 * @brief serialize message into protobuf
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @return serialized message
 * @throws std::runtime_error on error
 * @example `auto serialized_message = spb::pb::serialize(message);`
 */
template <spb::resizable_container Container>
auto serialize(const auto &message, const serialize_options &options) -> Container;

/**
 * @brief deserialize message from protobuf
 *
 * @param[in] reader function for handling reads
 * @param[in] options
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 */
void deserialize(auto &message, spb::io::reader reader, const deserialize_options &options);

/**
 * @brief deserialize message from protobuf
 *
 * @param[in] protobuf string with protobuf
 * @param[in] options
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 * @example `auto serialized = std::string( ... );`
 *          `auto message = Message();`
 *          `spb::pb::deserialize(message, serialized);`
 */
template <typename Message, spb::size_container Container>
void deserialize(Message &message, const Container &protobuf, const deserialize_options &options);

/**
 * @brief deserialize message from protobuf
 *
 * @param[in] protobuf serialized protobuf
 * @param[in] options
 * @return deserialized message
 * @throws std::runtime_error on error
 * @example `auto serialized = std::string( ... );`
 *          `auto message = spb::pb::deserialize<Message>(serialized);`
 */
template <typename Message, spb::size_container Container>
auto deserialize(const Container &protobuf, const deserialize_options &options) -> Message;

/**
 * @brief deserialize message from reader
 *
 * @param[in] reader function for handling reads
 * @param[in] options
 * @return deserialized message
 * @throws std::runtime_error on error
 */
template <typename Message>
auto deserialize(spb::io::reader reader, const deserialize_options &options) -> Message;

namespace detail
{
void serialize_value(ostream_size &, const ::ETL::Example::DeviceStatus &message);
void serialize_value(ostream_writer &, const ::ETL::Example::DeviceStatus &message);
void serialize_value(ostream_buffer &, const ::ETL::Example::DeviceStatus &message);
void deserialize_value(istream_reader &, ::ETL::Example::DeviceStatus &message, tag_type);
void deserialize_value(istream_buffer &, ::ETL::Example::DeviceStatus &message, tag_type);
void serialize_value(ostream_size &, const ::ETL::Example::Command &message);
void serialize_value(ostream_writer &, const ::ETL::Example::Command &message);
void serialize_value(ostream_buffer &, const ::ETL::Example::Command &message);
void deserialize_value(istream_reader &, ::ETL::Example::Command &message, tag_type);
void deserialize_value(istream_buffer &, ::ETL::Example::Command &message, tag_type);
void serialize_value(ostream_size &, const ::ETL::Example::CommandQueue &message);
void serialize_value(ostream_writer &, const ::ETL::Example::CommandQueue &message);
void serialize_value(ostream_buffer &, const ::ETL::Example::CommandQueue &message);
void deserialize_value(istream_reader &, ::ETL::Example::CommandQueue &message, tag_type);
void deserialize_value(istream_buffer &, ::ETL::Example::CommandQueue &message, tag_type);
} // namespace detail
} // namespace spb::pb
namespace spb::json::detail
{
struct ostream;
struct istream;

void serialize_value(ostream &, const ::ETL::Example::DeviceStatus &);
void deserialize_value(istream &, ::ETL::Example::DeviceStatus &);

void serialize_value(ostream &, const ::ETL::Example::Command &);
void deserialize_value(istream &, ::ETL::Example::Command &);

void serialize_value(ostream &, const ::ETL::Example::CommandQueue &);
void deserialize_value(istream &, ::ETL::Example::CommandQueue &);

void serialize_value(ostream &, const ::ETL::Example::Command::CMD &);
void deserialize_value(istream &, ::ETL::Example::Command::CMD &);
} // namespace spb::json::detail
