#include "vendor_schema.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

namespace pdv_test {
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

const pdv::MessageCatalogGroup* find_group(
    const pdv::MessageCatalog& catalog,
    const std::string& group_name) {
    for (const auto& group : catalog.groups) {
        if (group.name == group_name) {
            return &group;
        }
    }
    return nullptr;
}

const pdv::MessageCatalogEntry* find_entry(
    const pdv::MessageCatalogGroup& group,
    const std::string& entry_name) {
    for (const auto& entry : group.entries) {
        if (entry.name == entry_name) {
            return &entry;
        }
    }
    return nullptr;
}

const pdv::VendorField* find_field(
    const pdv::VendorMessage& message,
    const std::string& field_name) {
    for (const auto& field : message.message.fields) {
        if (field.name == field_name) {
            return &field;
        }
    }
    return nullptr;
}

bool contains(std::string_view haystack, std::string_view needle) {
    return haystack.find(needle) != std::string_view::npos;
}

class TempYamlFile {
public:
    explicit TempYamlFile(const std::string& contents) {
        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = fs::temp_directory_path() / ("vendor_schema_loader_test_" + std::to_string(now) + ".yaml");

        std::ofstream out(path_);
        if (!out) {
            throw std::runtime_error("Failed to create temporary YAML file");
        }
        out << contents;
        out.close();
    }

    ~TempYamlFile() {
        std::error_code ec;
        fs::remove(path_, ec);
    }

    [[nodiscard]] const std::string path() const {
        return path_.string();
    }

private:
    fs::path path_;
};

}  // namespace
}  // namespace pdv_test

TEST(VendorSchemaLoaderTest, LoadsOuch42MessageCatalog) {
    const auto catalog = pdv::load_message_catalog(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/message_catalog.yaml"
        }));

    EXPECT_EQ(catalog.protocol, "nasdaq-ouch");
    ASSERT_TRUE(catalog.version.has_value());
    EXPECT_EQ(*catalog.version, "4.2");
    EXPECT_EQ(catalog.spec_title, "O*U*C*H Version 4.2");
    EXPECT_EQ(catalog.catalog_style, "vendor");

    const auto* inbound = pdv_test::find_group(catalog, "inbound");
    ASSERT_NE(inbound, nullptr);
    EXPECT_GE(inbound->entries.size(), 4u);

    const auto* enter_order = pdv_test::find_entry(*inbound, "Enter Order Message");
    ASSERT_NE(enter_order, nullptr);
    ASSERT_TRUE(enter_order->type_code.has_value());
    EXPECT_EQ(*enter_order->type_code, "O");
    ASSERT_TRUE(enter_order->direction.has_value());
    EXPECT_EQ(*enter_order->direction, "client_to_exchange");
    EXPECT_EQ(enter_order->file, "enter_order.yaml");

    const auto* outbound = pdv_test::find_group(catalog, "outbound_sequenced");
    ASSERT_NE(outbound, nullptr);
    EXPECT_FALSE(outbound->entries.empty());

    const auto* accepted = pdv_test::find_entry(*outbound, "Accepted Message");
    ASSERT_NE(accepted, nullptr);
    ASSERT_TRUE(accepted->type_code.has_value());
    EXPECT_EQ(*accepted->type_code, "A");
}

TEST(VendorSchemaLoaderTest, LoadsOuch50MessageCatalog) {
    const auto catalog = pdv::load_message_catalog(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/message_catalog.yaml"
        }));

    EXPECT_EQ(catalog.protocol, "nasdaq-ouch");
    ASSERT_TRUE(catalog.version.has_value());
    EXPECT_EQ(*catalog.version, "5.0");
    EXPECT_EQ(catalog.spec_title, "OUCH 5.0 Order Entry Specification");

    const auto* inbound = pdv_test::find_group(catalog, "inbound");
    ASSERT_NE(inbound, nullptr);

    const auto* enter_order = pdv_test::find_entry(*inbound, "Enter Order");
    ASSERT_NE(enter_order, nullptr);
    ASSERT_TRUE(enter_order->type_code.has_value());
    EXPECT_EQ(*enter_order->type_code, "O");

    const auto* mass_cancel = pdv_test::find_entry(*inbound, "Mass Cancel Request");
    ASSERT_NE(mass_cancel, nullptr);
    ASSERT_TRUE(mass_cancel->type_code.has_value());
    EXPECT_EQ(*mass_cancel->type_code, "C");

    const auto* outbound = pdv_test::find_group(catalog, "outbound");
    ASSERT_NE(outbound, nullptr);
    EXPECT_FALSE(outbound->entries.empty());
}

TEST(VendorSchemaLoaderTest, LoadsNyseEquitiesBinaryCatalog) {
    const auto catalog = pdv::load_message_catalog(
        pdv_test::resolve_existing_path({
            "specs/vendor/nyse/pillar/equities/binary/messages/message_catalog.yaml"
        }));

    EXPECT_EQ(catalog.protocol, "nyse-pillar-gateway-binary");
    ASSERT_TRUE(catalog.spec_version.has_value());
    EXPECT_EQ(*catalog.spec_version, "5.17");
    ASSERT_TRUE(catalog.protocol_version.has_value());
    EXPECT_EQ(*catalog.protocol_version, "1.1");
    EXPECT_EQ(catalog.catalog_style, "vendor");
    EXPECT_FALSE(catalog.market_scope.empty());

    const auto* member_to_pillar = pdv_test::find_group(catalog, "member_firm_to_pillar");
    ASSERT_NE(member_to_pillar, nullptr);

    const auto* new_order = pdv_test::find_entry(
        *member_to_pillar, "New Order Single and Cancel/Replace Request");
    ASSERT_NE(new_order, nullptr);
    EXPECT_EQ(new_order->file, "new_order_single_and_cancel_replace_request.yaml");

    const auto* pillar_to_member = pdv_test::find_group(catalog, "pillar_to_member_firm");
    ASSERT_NE(pillar_to_member, nullptr);

    const auto* exec_report = pdv_test::find_entry(*pillar_to_member, "Execution Report");
    ASSERT_NE(exec_report, nullptr);
    EXPECT_EQ(exec_report->file, "execution_report.yaml");
}

TEST(VendorSchemaLoaderTest, LoadsNyseOptionsBinaryCatalog) {
    const auto catalog = pdv::load_message_catalog(
        pdv_test::resolve_existing_path({
            "specs/vendor/nyse/pillar/options/binary/messages/message_catalog.yaml"
        }));

    EXPECT_EQ(catalog.protocol, "nyse-pillar-options-gateway-binary");
    ASSERT_TRUE(catalog.spec_version.has_value());
    EXPECT_EQ(*catalog.spec_version, "3.26");
    ASSERT_TRUE(catalog.protocol_version.has_value());
    EXPECT_EQ(*catalog.protocol_version, "1.1");

    const auto* member_to_pillar = pdv_test::find_group(catalog, "member_firm_to_pillar");
    ASSERT_NE(member_to_pillar, nullptr);

    const auto* bulk_cancel_variant = pdv_test::find_entry(
        *member_to_pillar, "Bulk Cancel Request - 0x0223");
    ASSERT_NE(bulk_cancel_variant, nullptr);
    ASSERT_TRUE(bulk_cancel_variant->message_type_hex.has_value());
    EXPECT_EQ(*bulk_cancel_variant->message_type_hex, "0x0223");

    const auto* pillar_to_member = pdv_test::find_group(catalog, "pillar_to_member_firm");
    ASSERT_NE(pillar_to_member, nullptr);

    const auto* quote_ack_variant = pdv_test::find_entry(
        *pillar_to_member, "QuoteAck - 0x0308");
    ASSERT_NE(quote_ack_variant, nullptr);
    ASSERT_TRUE(quote_ack_variant->message_type_hex.has_value());
    EXPECT_EQ(*quote_ack_variant->message_type_hex, "0x0308");
}

TEST(VendorSchemaLoaderTest, LoadsOuch42EnterOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/enter_order.yaml"
        }));

    EXPECT_EQ(msg.protocol, "nasdaq-ouch");
    EXPECT_EQ(msg.version, "4.2");
    EXPECT_EQ(msg.source_section, "2.1");
    EXPECT_EQ(msg.catalog_style, "vendor");

    EXPECT_EQ(msg.message.name, "Enter Order Message");
    EXPECT_EQ(msg.message.type_code, "O");
    EXPECT_EQ(msg.message.direction, "client_to_exchange");
    ASSERT_FALSE(msg.message.fields.empty());

    const auto* type = pdv_test::find_field(msg, "Type");
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->offset, 0);
    EXPECT_EQ(type->length, 1);
    EXPECT_EQ(type->wire_type, "alpha");
    ASSERT_TRUE(type->fixed_value.has_value());
    EXPECT_EQ(*type->fixed_value, "O");

    const auto* order_token = pdv_test::find_field(msg, "Order Token");
    ASSERT_NE(order_token, nullptr);
    EXPECT_EQ(order_token->offset, 1);
    EXPECT_EQ(order_token->length, 14);
    EXPECT_EQ(order_token->wire_type, "token");

    const auto* display = pdv_test::find_field(msg, "Display");
    ASSERT_NE(display, nullptr);
    EXPECT_EQ(display->offset, 40);
    EXPECT_EQ(display->length, 1);
    EXPECT_FALSE(display->enum_values.empty());

    const auto* customer_type = pdv_test::find_field(msg, "Customer Type");
    ASSERT_NE(customer_type, nullptr);
    EXPECT_EQ(customer_type->offset, 48);
    EXPECT_EQ(customer_type->length, 1);
}

TEST(VendorSchemaLoaderTest, LoadsOuch50EnterOrderMessageAndVarLengthAppendage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/enter_order.yaml"
        }));

    EXPECT_EQ(msg.protocol, "nasdaq-ouch");
    EXPECT_EQ(msg.version, "5.0");
    EXPECT_EQ(msg.source_section, "2.1");
    EXPECT_EQ(msg.message.name, "Enter Order");
    EXPECT_EQ(msg.message.type_code, "O");
    EXPECT_EQ(msg.message.direction, "client_to_exchange");

    const auto* user_ref_num = pdv_test::find_field(msg, "UserRefNum");
    ASSERT_NE(user_ref_num, nullptr);
    EXPECT_EQ(user_ref_num->offset, 1);
    EXPECT_EQ(user_ref_num->length, 4);
    EXPECT_EQ(user_ref_num->wire_type, "user_ref_num");

    const auto* price = pdv_test::find_field(msg, "Price");
    ASSERT_NE(price, nullptr);
    EXPECT_EQ(price->offset, 18);
    EXPECT_EQ(price->length, 8);
    EXPECT_EQ(price->wire_type, "price");

    const auto* appendage_length = pdv_test::find_field(msg, "Appendage Length");
    ASSERT_NE(appendage_length, nullptr);
    EXPECT_EQ(appendage_length->offset, 45);
    EXPECT_EQ(appendage_length->length, 2);

    const auto* optional_appendage = pdv_test::find_field(msg, "Optional Appendage");
    ASSERT_NE(optional_appendage, nullptr);
    EXPECT_EQ(optional_appendage->offset, 47);
    EXPECT_EQ(optional_appendage->length, -1);
    EXPECT_EQ(optional_appendage->wire_type, "tag_value");
    EXPECT_FALSE(optional_appendage->appendage_options.empty());
}

TEST(VendorSchemaLoaderTest, LoadsOuch42ReplaceOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/replace_order.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Replace Order Message");
    EXPECT_EQ(msg.message.type_code, "U");

    const auto* existing_token = pdv_test::find_field(msg, "Existing Order Token");
    ASSERT_NE(existing_token, nullptr);
    EXPECT_EQ(existing_token->offset, 1);
    EXPECT_EQ(existing_token->length, 14);

    const auto* replacement_token = pdv_test::find_field(msg, "Replacement Order Token");
    ASSERT_NE(replacement_token, nullptr);
    EXPECT_EQ(replacement_token->offset, 15);
    EXPECT_EQ(replacement_token->length, 14);

    const auto* minimum_quantity = pdv_test::find_field(msg, "Minimum Quantity");
    ASSERT_NE(minimum_quantity, nullptr);
    EXPECT_EQ(minimum_quantity->offset, 43);
    EXPECT_EQ(minimum_quantity->length, 4);
}

TEST(VendorSchemaLoaderTest, LoadsOuch50ReplaceOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/replace_order.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/replace_order_request.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Replace Order Request");
    EXPECT_EQ(msg.message.type_code, "U");

    const auto* orig_user_ref_num = pdv_test::find_field(msg, "OrigUserRefNum");
    ASSERT_NE(orig_user_ref_num, nullptr);
    EXPECT_EQ(orig_user_ref_num->offset, 1);
    EXPECT_EQ(orig_user_ref_num->length, 4);

    const auto* user_ref_num = pdv_test::find_field(msg, "UserRefNum");
    ASSERT_NE(user_ref_num, nullptr);
    EXPECT_EQ(user_ref_num->offset, 5);
    EXPECT_EQ(user_ref_num->length, 4);

    const auto* cl_ord_id = pdv_test::find_field(msg, "ClOrdID");
    ASSERT_NE(cl_ord_id, nullptr);
    EXPECT_EQ(cl_ord_id->offset, 24);
    EXPECT_EQ(cl_ord_id->length, 14);

    const auto* optional_appendage = pdv_test::find_field(msg, "Optional Appendage");
    ASSERT_NE(optional_appendage, nullptr);
    EXPECT_EQ(optional_appendage->length, -1);
    EXPECT_FALSE(optional_appendage->appendage_options.empty());
}

TEST(VendorSchemaLoaderTest, LoadsOuch42CancelOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/cancel_order.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Cancel Order Message");
    EXPECT_EQ(msg.message.type_code, "X");
    EXPECT_FALSE(msg.message.behavior_notes.empty());

    const auto* order_token = pdv_test::find_field(msg, "Order Token");
    ASSERT_NE(order_token, nullptr);
    EXPECT_EQ(order_token->offset, 1);
    EXPECT_EQ(order_token->length, 14);

    const auto* shares = pdv_test::find_field(msg, "Shares");
    ASSERT_NE(shares, nullptr);
    EXPECT_EQ(shares->offset, 15);
    EXPECT_EQ(shares->length, 4);
}

TEST(VendorSchemaLoaderTest, LoadsOuch50CancelOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/cancel_order.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/cancel_order_request.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Cancel Order Request");
    EXPECT_EQ(msg.message.type_code, "X");

    const auto* user_ref_num = pdv_test::find_field(msg, "UserRefNum");
    ASSERT_NE(user_ref_num, nullptr);
    EXPECT_EQ(user_ref_num->offset, 1);
    EXPECT_EQ(user_ref_num->length, 4);

    const auto* quantity = pdv_test::find_field(msg, "Quantity");
    ASSERT_NE(quantity, nullptr);
    EXPECT_EQ(quantity->offset, 5);
    EXPECT_EQ(quantity->length, 4);

    const auto* appendage_length = pdv_test::find_field(msg, "Appendage Length");
    ASSERT_NE(appendage_length, nullptr);
    EXPECT_FALSE(appendage_length->required);

    const auto* optional_appendage = pdv_test::find_field(msg, "Optional Appendage");
    ASSERT_NE(optional_appendage, nullptr);
    EXPECT_FALSE(optional_appendage->required);
    EXPECT_EQ(optional_appendage->length, -1);
}

TEST(VendorSchemaLoaderTest, LoadsOuch42ModifyOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/4.2/messages/modify_order.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Modify Order Message");
    EXPECT_EQ(msg.message.type_code, "M");

    const auto* order_token = pdv_test::find_field(msg, "Order Token");
    ASSERT_NE(order_token, nullptr);
    EXPECT_EQ(order_token->offset, 1);
    EXPECT_EQ(order_token->length, 14);

    const auto* side = pdv_test::find_field(msg, "Buy/Sell Indicator");
    ASSERT_NE(side, nullptr);
    EXPECT_EQ(side->offset, 15);
    EXPECT_EQ(side->length, 1);
    EXPECT_FALSE(side->enum_values.empty());

    const auto* shares = pdv_test::find_field(msg, "Shares");
    ASSERT_NE(shares, nullptr);
    EXPECT_EQ(shares->offset, 16);
    EXPECT_EQ(shares->length, 4);
}

TEST(VendorSchemaLoaderTest, LoadsOuch50ModifyOrderMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/modify_order.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/modify_order_request.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Modify Order Request");
    EXPECT_EQ(msg.message.type_code, "M");

    const auto* user_ref_num = pdv_test::find_field(msg, "UserRefNum");
    ASSERT_NE(user_ref_num, nullptr);
    EXPECT_EQ(user_ref_num->offset, 1);
    EXPECT_EQ(user_ref_num->length, 4);

    const auto* side = pdv_test::find_field(msg, "Side");
    ASSERT_NE(side, nullptr);
    EXPECT_EQ(side->offset, 5);
    EXPECT_EQ(side->length, 1);

    const auto* quantity = pdv_test::find_field(msg, "Quantity");
    ASSERT_NE(quantity, nullptr);
    EXPECT_EQ(quantity->offset, 6);
    EXPECT_EQ(quantity->length, 4);

    const auto* optional_appendage = pdv_test::find_field(msg, "Optional Appendage");
    ASSERT_NE(optional_appendage, nullptr);
    EXPECT_EQ(optional_appendage->length, -1);
    EXPECT_FALSE(optional_appendage->appendage_options.empty());
}

TEST(VendorSchemaLoaderTest, LoadsOuch50MassCancelMessage) {
    const auto msg = pdv::load_vendor_message(
        pdv_test::resolve_existing_path({
            "specs/vendor/nasdaq/ouch/5.0/messages/mass_cancel.yaml",
            "specs/vendor/nasdaq/ouch/5.0/messages/mass_cancel_request.yaml"
        }));

    EXPECT_EQ(msg.message.name, "Mass Cancel Request");
    EXPECT_EQ(msg.message.type_code, "C");

    const auto* user_ref_num = pdv_test::find_field(msg, "User Reference Number");
    ASSERT_NE(user_ref_num, nullptr);
    EXPECT_EQ(user_ref_num->offset, 1);
    EXPECT_EQ(user_ref_num->length, 4);

    const auto* symbol = pdv_test::find_field(msg, "Symbol");
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->offset, 9);
    EXPECT_EQ(symbol->length, 8);
    EXPECT_FALSE(symbol->required);

    const auto* optional_appendage = pdv_test::find_field(msg, "Optional Appendage");
    ASSERT_NE(optional_appendage, nullptr);
    EXPECT_EQ(optional_appendage->offset, 19);
    EXPECT_EQ(optional_appendage->length, -1);
    EXPECT_FALSE(optional_appendage->appendage_options.empty());
}

TEST(VendorSchemaLoaderTest, ThrowsHelpfulErrorForMissingRequiredCatalogKey) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
spec_title: "Broken Catalog"
source_pdf: "broken.pdf"
catalog_style: "vendor"
messages:
  inbound:
    - section: "1"
      name: "Message"
      file: "message.yaml"
)yaml");

    try {
        (void)pdv::load_message_catalog(temp_yaml.path());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& ex) {
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Failed to load message catalog"));
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Missing required key: protocol"));
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(VendorSchemaLoaderTest, ThrowsHelpfulErrorForInvalidMessagesNodeType) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
protocol: "nasdaq-ouch"
version: "4.2"
spec_title: "Broken Catalog"
source_pdf: "broken.pdf"
catalog_style: "vendor"
messages:
  - "not a map"
)yaml");

    try {
        (void)pdv::load_message_catalog(temp_yaml.path());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& ex) {
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Failed to load message catalog"));
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Key 'messages' must be a YAML map"));
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(VendorSchemaLoaderTest, ThrowsHelpfulErrorForMissingMessageBlock) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
protocol: "nasdaq-ouch"
version: "4.2"
spec_title: "Broken Message"
source_pdf: "broken.pdf"
source_section: "2.1"
catalog_style: "vendor"
)yaml");

    try {
        (void)pdv::load_vendor_message(temp_yaml.path());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& ex) {
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Failed to load vendor message"));
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Missing required key: message"));
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(VendorSchemaLoaderTest, ThrowsHelpfulErrorForInvalidLengthValue) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
protocol: "nasdaq-ouch"
version: "5.0"
spec_title: "Broken Message"
source_pdf: "broken.pdf"
source_section: "2.1"
catalog_style: "vendor"
message:
  name: "Broken Message"
  type_code: "O"
  direction: "client_to_exchange"
  fields:
    - name: "Bad Field"
      offset: 0
      length: "banana"
      wire_type: "alpha"
)yaml");

    try {
        (void)pdv::load_vendor_message(temp_yaml.path());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& ex) {
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Failed to load vendor message"));
        EXPECT_TRUE(pdv_test::contains(ex.what(), "Invalid length value for key: length"));
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(VendorSchemaLoaderTest, DefaultsRequiredToTrueWhenFieldOmitsRequiredFlag) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
protocol: "example"
version: "1.0"
spec_title: "Example Spec"
source_pdf: "example.pdf"
source_section: "1.1"
catalog_style: "vendor"
message:
  name: "Simple Message"
  type_code: "Z"
  direction: "client_to_exchange"
  fields:
    - name: "Type"
      offset: 0
      length: 1
      wire_type: "alpha"
      fixed_value: "Z"
)yaml");

    const auto msg = pdv::load_vendor_message(temp_yaml.path());
    ASSERT_EQ(msg.message.fields.size(), 1u);
    EXPECT_TRUE(msg.message.fields.front().required);
}

TEST(VendorSchemaLoaderTest, ParsesEnumValuesAndBehaviorNotes) {
    const pdv_test::TempYamlFile temp_yaml(R"yaml(
protocol: "example"
version: "1.0"
spec_title: "Example Spec"
source_pdf: "example.pdf"
source_section: "1.1"
catalog_style: "vendor"
message:
  name: "Enum Message"
  type_code: "E"
  direction: "client_to_exchange"
  summary: "Example"
  behavior_notes:
    - "First note"
    - "Second note"
  fields:
    - name: "Side"
      offset: 0
      length: 1
      wire_type: "alpha"
      enum_values:
        - value: "B"
          meaning: "buy"
        - value: "S"
          meaning: "sell"
)yaml");

    const auto msg = pdv::load_vendor_message(temp_yaml.path());

    ASSERT_EQ(msg.message.behavior_notes.size(), 2u);
    EXPECT_EQ(msg.message.behavior_notes[0], "First note");
    EXPECT_EQ(msg.message.behavior_notes[1], "Second note");

    ASSERT_EQ(msg.message.fields.size(), 1u);
    const auto& field = msg.message.fields.front();
    ASSERT_EQ(field.enum_values.size(), 2u);
    EXPECT_EQ(field.enum_values[0].value, "B");
    EXPECT_EQ(field.enum_values[0].meaning, "buy");
    EXPECT_EQ(field.enum_values[1].value, "S");
    EXPECT_EQ(field.enum_values[1].meaning, "sell");
}