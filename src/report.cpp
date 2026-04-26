#include "report.hpp"

#include <sstream>
#include <string>

namespace pdv {

std::string diff_kind_to_string(DiffKind kind) {
    switch (kind) {
        case DiffKind::AddedField:
            return "Added field";
        case DiffKind::RemovedField:
            return "Removed field";
        case DiffKind::OffsetChanged:
            return "Offset changed";
        case DiffKind::LengthChanged:
            return "Length changed";
        case DiffKind::WireTypeChanged:
            return "Wire type changed";
        case DiffKind::RequirednessChanged:
            return "Requiredness changed";
    }

    return "Unknown diff";
}

std::string render_message_diff_report(const MessageDiff& diff) {
    std::ostringstream out;

    out << "Message diff: "
        << diff.old_message_name
        << " -> "
        << diff.new_message_name
        << "\n";

    if (diff.field_diffs.empty()) {
        out << "No field-level differences detected.\n";
        return out.str();
    }

    for (const auto& field_diff : diff.field_diffs) {
        out << "- "
            << diff_kind_to_string(field_diff.kind)
            << ": "
            << field_diff.field_name;

        if (!field_diff.details.empty()) {
            out << " (" << field_diff.details << ")";
        }

        out << "\n";
    }

    return out.str();
}

}  // namespace pdv