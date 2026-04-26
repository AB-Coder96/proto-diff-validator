#pragma once

#include "diff.hpp"

#include <string>

namespace pdv {


std::string diff_kind_to_string(DiffKind kind);

std::string render_message_diff_report(const MessageDiff& diff);

}  