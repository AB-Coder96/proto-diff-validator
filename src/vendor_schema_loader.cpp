#include "vendor_schema.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace pdv {
namespace {

[[nodiscard]] std::string require_string(const YAML::Node& node, const char* key) {
    if (!node[key]) {
        throw std::runtime_error(std::string("Missing required key: ") + key);
    }
    return node[key].as<std::string>();
}

[[nodiscard]] std::optional<std::string> optional_string(const YAML::Node& node, const char* key) {
    if (!node[key]) {
        return std::nullopt;
    }
    return node[key].as<std::string>();
}

[[nodiscard]] std::vector<std::string> optional_string_list(const YAML::Node& node, const char* key) {
    std::vector<std::string> out;
    if (!node[key]) {
        return out;
    }

    for (const auto& item : node[key]) {
        out.push_back(item.as<std::string>());
    }
    return out;
}

[[nodiscard]] int parse_length(const YAML::Node& node, const char* key) {
    if (!node[key]) {
        throw std::runtime_error(std::string("Missing required key: ") + key);
    }

    const YAML::Node value = node[key];

    if (value.IsScalar()) {
        try {
            return value.as<int>();
        } catch (const YAML::BadConversion&) {
            const std::string s = value.as<std::string>();
            if (s == "var") {
                return -1;
            }
        }
    }

    throw std::runtime_error(std::string("Invalid length value for key: ") + key);
}

[[nodiscard]] std::vector<VendorEnumValue> parse_enum_values(const YAML::Node& field_node) {
    std::vector<VendorEnumValue> out;

    if (!field_node["enum_values"]) {
        return out;
    }

    for (const auto& enum_node : field_node["enum_values"]) {
        VendorEnumValue entry;
        entry.value = require_string(enum_node, "value");
        entry.meaning = require_string(enum_node, "meaning");
        out.push_back(std::move(entry));
    }

    return out;
}

[[nodiscard]] VendorField parse_field(const YAML::Node& field_node) {
    VendorField field;
    field.name = require_string(field_node, "name");

    if (field_node["offset"]) {
        field.offset = field_node["offset"].as<int>();
    } else {
        throw std::runtime_error("Missing required key: offset");
    }

    field.length = parse_length(field_node, "length");
    field.wire_type = require_string(field_node, "wire_type");
    field.required = field_node["required"] ? field_node["required"].as<bool>() : true;
    field.fixed_value = optional_string(field_node, "fixed_value");
    field.enum_values = parse_enum_values(field_node);
    field.appendage_options = optional_string_list(field_node, "appendage_options");
    field.notes = optional_string_list(field_node, "notes");

    return field;
}

[[nodiscard]] MessageCatalogEntry parse_catalog_entry(const YAML::Node& entry_node) {
    MessageCatalogEntry entry;
    entry.section = require_string(entry_node, "section");
    entry.name = require_string(entry_node, "name");
    entry.type_code = optional_string(entry_node, "type_code");
    entry.direction = optional_string(entry_node, "direction");
    entry.message_type_hex = optional_string(entry_node, "message_type_hex");
    entry.file = require_string(entry_node, "file");
    return entry;
}

[[nodiscard]] MessageCatalogGroup parse_catalog_group(const std::string& group_name, const YAML::Node& group_node) {
    if (!group_node.IsSequence()) {
        throw std::runtime_error("Catalog group '" + group_name + "' must be a YAML sequence");
    }

    MessageCatalogGroup group;
    group.name = group_name;

    for (const auto& entry_node : group_node) {
        group.entries.push_back(parse_catalog_entry(entry_node));
    }

    return group;
}

[[nodiscard]] std::runtime_error wrap_yaml_error(
    const std::string& path,
    const std::string& context,
    const std::exception& ex) {
    std::ostringstream oss;
    oss << "Failed to load " << context << " from '" << path << "': " << ex.what();
    return std::runtime_error(oss.str());
}

}  // namespace

MessageCatalog load_message_catalog(const std::string& path) {
    try {
        const YAML::Node root = YAML::LoadFile(path);

        MessageCatalog catalog;
        catalog.protocol = require_string(root, "protocol");
        catalog.version = optional_string(root, "version");
        catalog.spec_version = optional_string(root, "spec_version");
        catalog.protocol_version = optional_string(root, "protocol_version");
        catalog.spec_title = require_string(root, "spec_title");
        catalog.source_pdf = require_string(root, "source_pdf");
        catalog.catalog_style = require_string(root, "catalog_style");
        catalog.catalog_basis = optional_string(root, "catalog_basis");
        catalog.market_scope = optional_string_list(root, "market_scope");
        catalog.message_families = optional_string_list(root, "message_families");
        catalog.notes = optional_string_list(root, "notes");

        if (!root["messages"]) {
            throw std::runtime_error("Missing required key: messages");
        }
        if (!root["messages"].IsMap()) {
            throw std::runtime_error("Key 'messages' must be a YAML map");
        }

        for (const auto& kv : root["messages"]) {
            const std::string group_name = kv.first.as<std::string>();
            const YAML::Node group_node = kv.second;
            catalog.groups.push_back(parse_catalog_group(group_name, group_node));
        }

        return catalog;
    } catch (const std::exception& ex) {
        throw wrap_yaml_error(path, "message catalog", ex);
    }
}

VendorMessage load_vendor_message(const std::string& path) {
    try {
        const YAML::Node root = YAML::LoadFile(path);

        VendorMessage out;
        out.protocol = require_string(root, "protocol");
        out.version = require_string(root, "version");
        out.spec_title = require_string(root, "spec_title");
        out.source_pdf = require_string(root, "source_pdf");
        out.source_section = require_string(root, "source_section");
        out.catalog_style = require_string(root, "catalog_style");

        if (!root["message"]) {
            throw std::runtime_error("Missing required key: message");
        }

        const YAML::Node msg = root["message"];
        out.message.name = require_string(msg, "name");
        out.message.type_code = require_string(msg, "type_code");
        out.message.direction = require_string(msg, "direction");
        out.message.summary = msg["summary"] ? msg["summary"].as<std::string>() : "";

        if (!msg["fields"]) {
            throw std::runtime_error("Missing required key: message.fields");
        }
        if (!msg["fields"].IsSequence()) {
            throw std::runtime_error("Key 'message.fields' must be a YAML sequence");
        }

        for (const auto& field_node : msg["fields"]) {
            out.message.fields.push_back(parse_field(field_node));
        }

        out.message.behavior_notes = optional_string_list(msg, "behavior_notes");

        return out;
    } catch (const std::exception& ex) {
        throw wrap_yaml_error(path, "vendor message", ex);
    }
}

}  