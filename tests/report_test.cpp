#include "diff.hpp"
#include "report.hpp"
#include "vendor_schema.hpp"

#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

namespace pdv {
VendorMessage load_vendor_message(const std::string& path);
}

namespace report_test {
namespace {

std::string resolve_existing_path(std::initializer_list<std::string> relative_candidates) {
    const std::vector<fs::path> prefixes = {
        fs::path("."),
        fs::path(".."),
        fs::path("../.."),
    };

    for (const auto& rel : relative_candidates) {
        for (const auto& prefix : prefixes) {
            const fs::path candidate = (prefix / rel).lexically_normal();
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }
    }

    ADD_FAILURE() << "Could not resolve any candidate path.";
    return std::string(*relative_candidates.begin());
}

}  // namespace
}  // namespace report_test

TEST(ReportTest, DiffKindToStringReturnsReadableLabels) {
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::AddedField), "Added field");
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::RemovedField), "Removed field");
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::OffsetChanged), "Offset changed");
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::LengthChanged), "Length changed");
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::WireTypeChanged), "Wire type changed");
    EXPECT_EQ(pdv::diff_kind_to_string(pdv::DiffKind::RequirednessChanged), "Requiredness changed");
}

TEST(ReportTest, RenderMessageDiffReportIncludesHeaderAndFieldDiffs) {
    const auto old_message = pdv::load_vendor_message(
        report_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/enter_order.yaml"
        }));

    const auto new_message = pdv::load_vendor_message(
        report_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/enter_order.yaml"
        }));

    const auto diff = pdv::diff_messages(old_message, new_message);
    const auto report = pdv::render_message_diff_report(diff);

    EXPECT_NE(report.find("Message diff: Enter Order Message -> Enter Order"), std::string::npos);

    EXPECT_NE(report.find("Removed field: Order Token"), std::string::npos);
    EXPECT_NE(report.find("Removed field: Buy/Sell Indicator"), std::string::npos);

    EXPECT_NE(report.find("Added field: UserRefNum"), std::string::npos);
    EXPECT_NE(report.find("Added field: Side"), std::string::npos);
    EXPECT_NE(report.find("Added field: ClOrdID"), std::string::npos);
    EXPECT_NE(report.find("Added field: Appendage Length"), std::string::npos);
    EXPECT_NE(report.find("Added field: Optional Appendage"), std::string::npos);

    EXPECT_NE(report.find("Length changed: Price"), std::string::npos);
}

TEST(ReportTest, RenderMessageDiffReportHandlesNoDifferences) {
    pdv::MessageDiff diff;
    diff.old_message_name = "System Event Message";
    diff.new_message_name = "System Event";

    const auto report = pdv::render_message_diff_report(diff);

    EXPECT_NE(report.find("Message diff: System Event Message -> System Event"), std::string::npos);
    EXPECT_NE(report.find("No field-level differences detected."), std::string::npos);
}

TEST(ReportTest, RenderMessageDiffReportIncludesDetailsWhenPresent) {
    pdv::MessageDiff diff;
    diff.old_message_name = "Old Message";
    diff.new_message_name = "New Message";
    diff.field_diffs.push_back(pdv::FieldDiff{
        .kind = pdv::DiffKind::LengthChanged,
        .field_name = "Price",
        .details = "length changed from '4' to '8'"
    });

    const auto report = pdv::render_message_diff_report(diff);

    EXPECT_NE(report.find("Message diff: Old Message -> New Message"), std::string::npos);
    EXPECT_NE(report.find("Length changed: Price (length changed from '4' to '8')"), std::string::npos);
}