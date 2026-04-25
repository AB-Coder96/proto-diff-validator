#pragma once

#include "vendor_schema.hpp"

#include <string>
#include <vector>

namespace pdv {

enum class DiffKind {
    AddedField,
    RemovedField,
    OffsetChanged,
    LengthChanged,
    WireTypeChanged,
    RequirednessChanged
};

struct FieldDiff {
    DiffKind kind;
    std::string field_name;
    std::string details;
};

struct MessageDiff {
    std::string old_message_name;
    std::string new_message_name;
    std::vector<FieldDiff> field_diffs;
};

MessageDiff diff_messages(const VendorMessage& old_message, const VendorMessage& new_message);

}  // namespace pdv