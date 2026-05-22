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
#include <vector>

namespace ETL::Example
{
struct DeviceStatus
{
    uint32_t device_id : 31;
    uint32_t is_online : 1;
    uint32_t last_heartbeat;
    std::array< char, 8 > firmware_version;
    etl::string< 16 > name;
};
struct Command
{
    enum class CMD : uint8_t
    {
        CMD_RST   = 1,
        CMD_READ  = 2,
        CMD_WRITE = 3,
        CMD_TST   = 4,
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
    etl::vector< Command, 16 > commands;
    etl::vector< DeviceStatus, 16 > statuses;
};
}// namespace ETL::Example

namespace spb::json::detail
{
struct ostream;
struct istream;

void serialize_value( ostream & stream, const ::ETL::Example::DeviceStatus & value );
void deserialize_value( istream & stream, ::ETL::Example::DeviceStatus & value );

void serialize_value( ostream & stream, const ::ETL::Example::Command & value );
void deserialize_value( istream & stream, ::ETL::Example::Command & value );

void serialize_value( ostream & stream, const ::ETL::Example::CommandQueue & value );
void deserialize_value( istream & stream, ::ETL::Example::CommandQueue & value );

void serialize_value( ostream & stream, const ::ETL::Example::Command::CMD & value );
void deserialize_value( istream & stream, ::ETL::Example::Command::CMD & value );

}// namespace spb::json::detail
namespace spb::pb::detail
{
struct ostream;
struct istream;

void serialize( ostream & stream, const ::ETL::Example::DeviceStatus & value );
void deserialize_value( istream & stream, ::ETL::Example::DeviceStatus & value, tag_type tag );

void serialize( ostream & stream, const ::ETL::Example::Command & value );
void deserialize_value( istream & stream, ::ETL::Example::Command & value, tag_type tag );

void serialize( ostream & stream, const ::ETL::Example::CommandQueue & value );
void deserialize_value( istream & stream, ::ETL::Example::CommandQueue & value, tag_type tag );

}// namespace spb::pb::detail
