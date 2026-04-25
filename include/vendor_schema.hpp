#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pdv {

struct VendorEnumValue {
    std::string value;
    std::string meaning;
};

struct VendorField {
    std::string name;

    int offset = -1;
    int length = -1; // Use -1 for variable-length fields such as appendages.
    std::string wire_type;
    bool required = true;
    std::optional<std::string> fixed_value;
    std::vector<VendorEnumValue> enum_values;
    std::vector<std::string> appendage_options;
    std::vector<std::string> notes;
};

struct VendorMessage {
    std::string protocol;
    std::string version;
    std::string spec_title;
    std::string source_pdf;
    std::string source_section;
    std::string catalog_style;

    struct MessageBody {
        std::string name;
        std::string type_code;
        std::string direction;
        std::string summary;

        std::vector<VendorField> fields;
        std::vector<std::string> behavior_notes;
    } message;
};

struct MessageCatalogEntry {
    std::string section;
    std::string name;

    std::optional<std::string> type_code;
    std::optional<std::string> direction;
    std::optional<std::string> message_type_hex;

    std::string file;
};

struct MessageCatalogGroup {
    std::string name;
    std::vector<MessageCatalogEntry> entries;
};

struct MessageCatalog {
    std::string protocol;

    std::optional<std::string> version;
    std::optional<std::string> spec_version;
    std::optional<std::string> protocol_version;

    std::string spec_title;
    std::string source_pdf;
    std::string catalog_style;

    std::optional<std::string> catalog_basis;
    std::vector<std::string> market_scope;
    std::vector<std::string> message_families;
    std::vector<std::string> notes;

    std::vector<MessageCatalogGroup> groups;
};

MessageCatalog load_message_catalog(const std::string& path);
VendorMessage load_vendor_message(const std::string& path);

} 