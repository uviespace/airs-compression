# AIRSPACE Compression Library {#mainpage}

**AIRSPACE** takes the air out of space science data.

<img src="AIRSPACE_Logo.svg" alt="AIRSPACE Logo" style="width: min(700px, 100%); height: auto;">

A lightweight C library for lossless data compression.

## Getting Started

Include `<cmp.h>` and `<cmp_errors.h>`, then see the
[examples](https://github.com/uviespace/airs-compression/tree/main/examples)
for a complete usage example.

The usual workflow is:

1. Configure the compression parameters (@ref configuration).
2. Calculate and allocate the required buffers (@ref helpers).
3. Initialize a compression context with the parameters and working buffer
   (@ref context).
4. Compress data and check the result for errors (@ref compression and
   @ref error_handling).
5. Reset, reuse, or clean up the context (@ref context).

## Design and Format

- @ref design "Compression Design"
- @ref format "Compression Header Format"

## API Reference

- @ref configuration "Configuration"
- @ref helpers "Buffer Sizing"
- @ref error_handling "Error Handling"
- @ref context "Compression Context"
- @ref compression "Compression"
- @ref header "Compression Header"

## Guides

- @ref installation_guide "Installation Guide"
- @ref cli_guide "Command-line Interface"
