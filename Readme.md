# proto-diff-validator

C++20 toolkit for diffing, validating, and benchmarking order-entry protocol upgrades across Nasdaq OUCH and NYSE Pillar Binary.

## Status

Work in progress.

The repository is currently in the **vendor-schema foundation** stage:
- official protocol PDFs are stored under `specs/vendor/.../source/`
- each in-scope protocol version has a `message_catalog.yaml`
- initial vendor-faithful message YAMLs are being added for comparable OUCH 4.2 and OUCH 5.0 order-entry messages
- the next implementation step is the YAML loader and vendor schema model in C++

## Why this project exists

Protocol upgrades are often handled through manual document review, ad hoc test edits, and one-off codec changes. That workflow does not scale well across multiple venues and multiple protocol revisions.

This project turns protocol upgrades into a repeatable engineering workflow:

**schema ingest → semantic diff → generated validation → benchmark → CI summary**

The goal is not to build a production exchange gateway.  
The goal is to build tooling that answers the operational questions engineers actually care about:

- What changed?
- What broke?
- What tests need to run?
- Did performance drift after the schema change?

## Scope

This repository is focused on **order-entry protocol evolution**, not market data.

Current protocol families in scope:
- Nasdaq OUCH
- NYSE Pillar Binary

Current repository source layer includes vendor documentation and message catalogs for:
- Nasdaq OUCH 4.2
- Nasdaq OUCH 5.0
- NYSE Pillar Gateway Binary Protocol Specification
- NYSE Pillar Options Gateway Binary Protocol Specification

## Repository approach

The project is intentionally **vendor-first** right now.

That means:
- protocol PDFs are preserved as source material
- message catalogs are maintained per protocol version
- message YAML files mirror vendor terminology and field ordering
- normalization and semantic comparison logic will happen in code, not by prematurely rewriting the schemas

This keeps the extraction layer traceable and makes it easy to verify the YAML against the original vendor spec.

## Current architecture direction

The target architecture is:

1. **Schema ingest**  
   Load vendor-derived YAML/JSON descriptions for supported protocol versions.

2. **Diff engine**  
   Compare message sets, field layouts, message identifiers, enums, defaults, optionality, and other compatibility-relevant attributes.

3. **Validation generator**  
   Emit regression cases, negative tests, and migration notes from the detected deltas.

4. **Benchmark harness**  
   Exercise generated validator/codec paths with a reproducible benchmark workload.

5. **CI workflow**  
   Run diff, generation, tests, and benchmarks on schema changes and publish a migration summary.

## What is implemented so far

### Source layer
- vendor PDF storage layout under `specs/vendor/.../source/`
- per-version `message_catalog.yaml`
- initial OUCH vendor message YAMLs for comparable messages such as:
  - `enter_order`
  - `replace_order`
  - `cancel_order`
  - `modify_order`
  - selected 5.0-only messages as needed

### Repository structure
- `specs/` for vendor docs and extracted schemas
- `include/` for schema model headers
- `src/` for loaders and diff logic
- `tests/` for parser and diff validation
- `bench/` for benchmark harness code

## Immediate next step

The next implementation milestone is the **vendor schema loading path**:

- define `include/vendor_schema.hpp`
- implement `src/vendor_schema_loader.cpp`
- add loader tests
- load one catalog and one message YAML end to end
- then build the first semantic diff on:
  - OUCH 4.2 `enter_order`
  - OUCH 5.0 `enter_order`

That first vertical slice will prove the core pipeline before the repository is expanded to broader message coverage.

## Design principles

- **vendor-faithful extraction first**
- **small end-to-end slices before broad coverage**
- **correctness before optimization**
- **explicit scope control**
- **benchmarks only after the diff/validation path is trustworthy**

## Non-goals

- production exchange gateway implementation
- market-data protocol support in this repository
- generic code generation for every exchange protocol
- UI-first scope

## Planned milestones

### Milestone 1
Vendor schema model and parser for the supported protocol versions.

### Milestone 2
Semantic diff engine with change classification and migration report output.

### Milestone 3
Generated golden vectors and negative tests for the locked message-template set.

### Milestone 4
Benchmark harness and CI integration with reproducible reporting.

## Repository layout

```text
.
├── README.md
├── CMakeLists.txt
├── include/
├── src/
├── tests/
├── bench/
└── specs/
    ├── README.md
    └── vendor/
        ├── nasdaq/
        │   └── ouch/
        │       ├── 4.2/
        │       └── 5.0/
        └── nyse/
            └── pillar/
                ├── equities/
                │   └── binary/
                └── options/
                    └── binary/