#include "diff.hpp"

#include <sstream>
#include <string>
#include <unordered_map>

namespace pdv {
namespace {

std::string make_changed_details(
    const std::string& attribute,
    const std::string& old_value,
    const std::string& new_value) {
    std::ostringstream oss;
    oss << attribute << " changed from '" << old_value << "' to '" << new_value << "'";
    return oss.str();
}

std::string bool_to_string(bool value) {
    return value ? "true" : "false";
}

}  

MessageDiff diff_messages(const VendorMessage& old_message, const VendorMessage& new_message) {
    MessageDiff result;
    result.old_message_name = old_message.message.name;
    result.new_message_name = new_message.message.name;

    std::unordered_map<std::string, const VendorField*> old_fields_by_name;
    std::unordered_map<std::string, const VendorField*> new_fields_by_name;

    for (const auto& field : old_message.message.fields) {
        old_fields_by_name[field.name] = &field;
    }

    for (const auto& field : new_message.message.fields) {
        new_fields_by_name[field.name] = &field;
    }

    // Walk old fields first so removals/changes preserve old-message order.
    for (const auto& old_field : old_message.message.fields) {
        const auto new_it = new_fields_by_name.find(old_field.name);

        if (new_it == new_fields_by_name.end()) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::RemovedField,
                .field_name = old_field.name,
                .details = "Field removed from new version"
            });
            continue;
        }

        const VendorField& new_field = *new_it->second;

        if (old_field.offset != new_field.offset) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::OffsetChanged,
                .field_name = old_field.name,
                .details = make_changed_details(
                    "offset",
                    std::to_string(old_field.offset),
                    std::to_string(new_field.offset))
            });
        }

        if (old_field.length != new_field.length) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::LengthChanged,
                .field_name = old_field.name,
                .details = make_changed_details(
                    "length",
                    std::to_string(old_field.length),
                    std::to_string(new_field.length))
            });
        }

        if (old_field.wire_type != new_field.wire_type) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::WireTypeChanged,
                .field_name = old_field.name,
                .details = make_changed_details(
                    "wire_type",
                    old_field.wire_type,
                    new_field.wire_type)
            });
        }

        if (old_field.required != new_field.required) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::RequirednessChanged,
                .field_name = old_field.name,
                .details = make_changed_details(
                    "required",
                    bool_to_string(old_field.required),
                    bool_to_string(new_field.required))
            });
        }
    }

    // Walk new fields second so additions preserve new-message order.
    for (const auto& new_field : new_message.message.fields) {
        if (old_fields_by_name.find(new_field.name) == old_fields_by_name.end()) {
            result.field_diffs.push_back(FieldDiff{
                .kind = DiffKind::AddedField,
                .field_name = new_field.name,
                .details = "Field added in new version"
            });
        }
    }

    return result;
}

} 