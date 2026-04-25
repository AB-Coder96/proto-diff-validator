#include "diff.hpp"
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

namespace diff_test {
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

const pdv::FieldDiff* find_diff(
    const pdv::MessageDiff& diff,
    pdv::DiffKind kind,
    const std::string& field_name) {
    for (const auto& field_diff : diff.field_diffs) {
        if (field_diff.kind == kind && field_diff.field_name == field_name) {
            return &field_diff;
        }
    }
    return nullptr;
}

}  // namespace
}  // namespace diff_test

TEST(DiffTest, DiffsOuchEnterOrderBetween42And50) {
    const auto old_message = pdv::load_vendor_message(
        diff_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/enter_order.yaml"
        }));

    const auto new_message = pdv::load_vendor_message(
        diff_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/enter_order.yaml"
        }));

    const auto diff = pdv::diff_messages(old_message, new_message);

    EXPECT_EQ(diff.old_message_name, "Enter Order Message");
    EXPECT_EQ(diff.new_message_name, "Enter Order");
    EXPECT_FALSE(diff.field_diffs.empty());

    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::RemovedField, "Order Token"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::RemovedField, "Buy/Sell Indicator"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "UserRefNum"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "Side"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "ClOrdID"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "Appendage Length"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "Optional Appendage"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::LengthChanged, "Price"), nullptr);
}

TEST(DiffTest, DiffsOuchReplaceOrderBetween42And50) {
    const auto old_message = pdv::load_vendor_message(
        diff_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/replace_order.yaml"
        }));

    const auto new_message = pdv::load_vendor_message(
        diff_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/replace_order.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/replace_order_request.yaml"
        }));

    const auto diff = pdv::diff_messages(old_message, new_message);

    EXPECT_FALSE(diff.field_diffs.empty());

    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::RemovedField, "Existing Order Token"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::RemovedField, "Replacement Order Token"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "OrigUserRefNum"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "UserRefNum"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "ClOrdID"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "Appendage Length"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::AddedField, "Optional Appendage"), nullptr);
    EXPECT_NE(diff_test::find_diff(diff, pdv::DiffKind::LengthChanged, "Price"), nullptr);
}

TEST(DiffTest, TreatsFiveZeroOnlyMessageAsLoadedButNotComparedHere) {
    const auto message = pdv::load_vendor_message(
        diff_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/mass_cancel.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/mass_cancel_request.yaml"
        }));

    EXPECT_EQ(message.message.type_code, "C");
    EXPECT_EQ(message.message.direction, "client_to_exchange");
    EXPECT_FALSE(message.message.fields.empty());
}