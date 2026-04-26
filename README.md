# proto-diff-validator

C++20 toolkit for loading, diffing, and reporting order-entry protocol schema changes across Nasdaq OUCH and NYSE Pillar Binary.

## Status

Working prototype.

The repository currently includes a tested vertical slice of the protocol-migration workflow:

- vendor-first schema extraction in YAML
- C++20 schema loader for catalogs and per-message definitions
- structural message diff engine
- human-readable message diff reports
- automated test coverage for loader, diff, and report layers

Current test status: **26/26 passing**.

## Why this project exists

Protocol upgrades are often handled with manual PDF review, ad hoc edits, and one-off tests. This project turns that process into a repeatable workflow:

**vendor schema -> parser -> diff -> report -> tests**
 
The goal is to build tooling that helps answer practical migration questions:

- What changed between protocol versions?
- Which fields were added, removed, or changed?
- Which messages exist only in one version?
- How can migration checks be made repeatable?

## Scope

This repository is focused on **order-entry protocol evolution**.

In scope at the source layer:

- Nasdaq OUCH 4.2
- Nasdaq OUCH 5.0
- NYSE Pillar Gateway Binary Protocol Specification
- NYSE Pillar Options Gateway Binary Protocol Specification

The implementation is currently deepest on the OUCH 4.2 vs 5.0 path.

## What is implemented

### Vendor schema layer

The repository stores vendor-derived YAML catalogs and message definitions under `specs/vendor/...`.

This layer is intentionally vendor-first:
- vendor message names are preserved inside YAML
- field order, offsets, lengths, and enums are kept close to the source specs
- normalization is deferred to code, not hard-coded into the schema files

### Loader

The loader parses:
- protocol message catalogs
- individual vendor message YAML files

Implemented in:
- `include/vendor_schema.hpp`
- `src/vendor_schema_loader.cpp`

### Diff engine

The current diff engine performs structural comparison between two loaded messages.

It currently detects:
- added fields
- removed fields
- offset changes
- length changes
- wire type changes
- requiredness changes

Implemented in:
- `include/diff.hpp`
- `src/diff.cpp`

### Report layer

The report layer renders a readable message-diff summary from the structural diff result.

Implemented in:
- `include/report.hpp`
- `src/report.cpp`

### Tests

The repository includes passing tests for:
- catalog loading
- message loading
- error handling in the loader
- first structural diff scenarios
- report formatting

## Current example coverage

Current tested OUCH message coverage includes:
- Enter Order
- Replace Order
- Cancel Order
- Modify Order
- Mass Cancel (5.0-only)

The NYSE side currently has tested catalog loading in place and is ready for deeper message-level expansion later.

## Example diff output

A current message diff report looks like this in spirit:

```text
Message diff: Enter Order Message -> Enter Order
- Removed field: Order Token
- Removed field: Buy/Sell Indicator
- Added field: UserRefNum
- Added field: Side
- Added field: ClOrdID
- Added field: Appendage Length
- Added field: Optional Appendage
- Length changed: Price (length changed from '4' to '8')
```

## Build and test

### Prerequisites

Example Ubuntu / WSL packages:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  gdb \
  git \
  ninja-build \
  pkg-config \
  libyaml-cpp-dev \
  libgtest-dev
```

### Configure and build

```bash
cmake -S . -B build
cmake --build build
```

### Run all tests

```bash
ctest --test-dir build --output-on-failure
```


### Vendor-first extraction

The repository currently prioritizes traceable extraction over aggressive normalization.

The project is being built through narrow, testable milestones rather than broad incomplete coverage.

That is why the repository currently has:
- deeper support for a few comparable OUCH messages
- lighter support for the broader NYSE source layer

## Current limitations

This is not finished migration tooling yet.

Not implemented yet:
- catalog-level version diff
- semantic rename detection
- compatibility classification
- generated regression / negative tests
- benchmark harness
- CI artifact publishing
- full locked-template coverage

## next steps

1. catalog-level diff between protocol versions
2. compatibility classification
3. full OUCH 4.2 vs 5.0 migration report
4. broader message-template coverage
5. generated validation artifacts
6. benchmark path


