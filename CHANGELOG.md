[![AIRSPACE Logo](docs/AIRSPACE_Logo.svg)](https://github.com/uviespace/airs-compression)


## [0.7.0](https://github.com/uviespace/airs-compression/compare/v0.6.0..v0.7.0) - 2026-02-18

### 🚀 Features

- *(cmp)* [**breaking**] Restructure compression header to fixed size format - ([cdf2067](https://github.com/uviespace/airs-compression/commit/cdf206779f634923cd49bad8d275ed4585b6efd1))
- *(cmp)* Add original\_dtype to header and refactor test fixtures - ([eed42e9](https://github.com/uviespace/airs-compression/commit/eed42e96f1aa6d98ee83c453dc622d8e82c182f5))

## [0.6.0](https://github.com/uviespace/airs-compression/compare/v0.5.0..v0.6.0) - 2026-02-02

### 🚀 Features

- *(cmp)* Add signed 16-bit integer compression support - ([4fc4390](https://github.com/uviespace/airs-compression/commit/4fc439043b748b37d834e5d4e790d75e8dec6863))
- *(cmp)* Add compression for i16 data packed in i32 - ([e4937b1](https://github.com/uviespace/airs-compression/commit/e4937b1c46e9ad17a9bc59d3c4403e01262b001e))

### ✨ Refactor

- *(compress)* Rename work\_buf\_needed and add model\_is\_needed helper - ([765a4a6](https://github.com/uviespace/airs-compression/commit/765a4a697d8be9288ce6ee9aafb93da367944a1e))

### 📚 Documentation

- *(cmp.h)* Clarify CMP\_UNCOMPRESSED\_BOUND() usage scenarios - ([cbb711a](https://github.com/uviespace/airs-compression/commit/cbb711a356442f01d12c35ca8413ee87c9acf02f))

### 🧪 Testing

- Parametrize tests for u16 and i16 compression - ([d0ca696](https://github.com/uviespace/airs-compression/commit/d0ca6965f1b861730925fd08075e3909dbb87b64))
- Sxtend test suite to cover i16\_in\_i32 compression - ([0a37569](https://github.com/uviespace/airs-compression/commit/0a37569a169dd3616fa20bfa370d745d14c4d5d9))

## [0.5.0](https://github.com/uviespace/airs-compression/compare/v0.4.0..v0.5.0) - 2026-01-20

### 🚀 Features

- *(cmp)* Add CMP\_UNCOMPRESSED\_BOUND macro - ([548bbca](https://github.com/uviespace/airs-compression/commit/548bbca6b163105f2145f9bff3f540e8dd2979db))

### 🐛 Bug Fixes

- *(cli)* Error message when output path is a directory - ([16743fe](https://github.com/uviespace/airs-compression/commit/16743fe08d11a0cd857a0262f82bd43a5b8a1a48))
- *(compress)* Correct IWT boundary logic and input validation - ([61b5e10](https://github.com/uviespace/airs-compression/commit/61b5e10f436c6854b4f3bd23617e760d8949e33f))
- Suppress snprintf warning on Linux ANSI C builds - ([cb2685a](https://github.com/uviespace/airs-compression/commit/cb2685a42f683f6eb212e82c30219917a73d11bc))

### ✨ Refactor

- *(test)* Use CMP\_UNCOMPRESSED\_BOUND macro in tests - ([c4dab3f](https://github.com/uviespace/airs-compression/commit/c4dab3f787dfbd4bcd0dd91e0a612e9a56053ff3))

### 📚 Documentation

- Clarify comments in cmp.h - ([cc16b61](https://github.com/uviespace/airs-compression/commit/cc16b61450cc5eab9d92cd46b70acb0a15ec0ac4))

### ⚙️ Miscellaneous Tasks

- *(cli)* Add Dockerfile and .dockerignore - ([a221f05](https://github.com/uviespace/airs-compression/commit/a221f05dfd9138c74fd43fca9c04beac0248342e))
- Add -Wflex-array-member-not-at-end warring - ([e420f18](https://github.com/uviespace/airs-compression/commit/e420f18bab23acb2f72d340ce3efc094b730ef18))
- Bump version to v0.5.0 - ([f95c710](https://github.com/uviespace/airs-compression/commit/f95c7102082e69e9cbb3db15b0ea12a420e02ef7))

## [0.4.0](https://github.com/uviespace/airs-compression/compare/v0.3.0..v0.4.0) - 2025-10-22

### 🚀 Features

- *(cli)* Implement compression parameter parsing - ([74cd818](https://github.com/uviespace/airs-compression/commit/74cd81828ae923f158b529f0b71dcaf590aab94c))
- *(lib/cmp.h)* [**breaking**] Make timestamp provider API explicit - ([0e3fc85](https://github.com/uviespace/airs-compression/commit/0e3fc85692f1fe939af96d2eb0d0515bd0f3add4))
- Add BCC2 GR712RC cross-compile support - ([cd7a23d](https://github.com/uviespace/airs-compression/commit/cd7a23dd442adcff25933e9be0d181dc53fe0568))

### 🐛 Bug Fixes

- Improve compression error log formatting - ([0134f6e](https://github.com/uviespace/airs-compression/commit/0134f6e69590b9e41c84e716c0ffe44335fc1f07))

### ✨ Refactor

- *(examples)* Update example to use model compression - ([a376a64](https://github.com/uviespace/airs-compression/commit/a376a6410c7fc416c300572400f40a41752508b8))

### 📚 Documentation

- Improve INSTALL.adoc readability and build setup descriptions - ([fad1ff0](https://github.com/uviespace/airs-compression/commit/fad1ff0a034ce0e71ea7639420edeb460b84a342))
- Add git-cliff for automated changelog generation - ([61d40ea](https://github.com/uviespace/airs-compression/commit/61d40ea004b66b3315f18275d1028aa0dafd46ea))
- Add initial CHANGELOG.md - ([b09b55d](https://github.com/uviespace/airs-compression/commit/b09b55d2d216bd114a7cccf30e08d9f152b62529))
- Refine and clarify API documentation - ([3e80311](https://github.com/uviespace/airs-compression/commit/3e803119d6f54b417d806973fcc16bcca84abd57))

### ⚡ Performance

- *(bitstream)* Optimize put\_be64 with aligned writes - ([4657b89](https://github.com/uviespace/airs-compression/commit/4657b89882be4a171d29d50d5fad65b21251600f))
- *(cmp)* Convert bitstream and encoder to sticky error handling - ([f91bb8b](https://github.com/uviespace/airs-compression/commit/f91bb8b6cfa0d9db3ced098096c08409d7a59ab7))
- *(encoder)* Optimize Golomb zero-escape sequence write - ([3951def](https://github.com/uviespace/airs-compression/commit/3951def65f12dc99acf6ff59fcca3d3235672e9e))

### 🎨 Styling

- Update and apply consistent code formatting with clang-format - ([b41974a](https://github.com/uviespace/airs-compression/commit/b41974ac27f9b671811e56fde7845bac34820aa3))

### 🧪 Testing

- Fix false positive leak sanitizer on Appel silicon - ([015fde6](https://github.com/uviespace/airs-compression/commit/015fde604c3455f3eb4b9c252f16f15e463048c6))

### ⚙️ Miscellaneous Tasks

- Fix typos - ([21c8873](https://github.com/uviespace/airs-compression/commit/21c887346dddf69877ac84b680e5e2cd89f1e8a7))
- Refactor compiler flags to handle unit tests - ([80de616](https://github.com/uviespace/airs-compression/commit/80de616746923a3d9822962dff936162f791010e))
- Define \_POSIX\_C\_SOURCE=200809 for snprintf - ([99744d0](https://github.com/uviespace/airs-compression/commit/99744d09e4dc61f74d31c70116fddc0361d305d3))
- Automate releases with GitHub Actions - ([def7947](https://github.com/uviespace/airs-compression/commit/def79470b99765e7acc720c3e2671809576651b8))

## [0.3.0](https://github.com/uviespace/airs-compression/compare/v0.2.0..v0.3.0) - 2025-06-23

### 🚀 Features

- *(cmp)* Support distinct primary/secondary encoder settings - ([e1afe3c](https://github.com/uviespace/airs-compression/commit/e1afe3cc3ed997deddb75bcbe7a2a3ee354c0848))
- *(cmp)* Validate encoder params in initialisation - ([47bf09a](https://github.com/uviespace/airs-compression/commit/47bf09a11a91bd6854ae2e20ba3e4cebc7cf148d))
- *(cmp)* Add Golomb multi-escape encoder - ([cf1c57c](https://github.com/uviespace/airs-compression/commit/cf1c57ca9a6599e557e3c7e1df7d7ad163aff99b))
- *(cmp)* Add optional checksum support for compressed data - ([53edad4](https://github.com/uviespace/airs-compression/commit/53edad47e0f8b888e67f9a9bbed86a9399f5f7f0))
- *(cmp)* Implement uncompressed fallback for ineffective compression - ([b6cc3fd](https://github.com/uviespace/airs-compression/commit/b6cc3fd8df6cddf2bf8a6a4b991d43486af4feb8))
- *(cmp)* Add magic number validation to prevent use of uninitialized contexts - ([e6b297d](https://github.com/uviespace/airs-compression/commit/e6b297df1d4f0df9751f2f467929c58915a5fd9a))
- *(compress)* Implement data preprocessing for improved compression - ([0e5ef73](https://github.com/uviespace/airs-compression/commit/0e5ef7378c784bb57aec2a95e435eec0268e1685))
- *(compress)* Implement model-based preprocessing - ([80bc017](https://github.com/uviespace/airs-compression/commit/80bc0173a4b50f9c0ac9b9066562273589c85c74))
- *(docs)* Integrate Doxygen documentation generation into build system - ([895cf33](https://github.com/uviespace/airs-compression/commit/895cf33907f614344cbfb5a335d393790807a204))
- *(errors)* Add CMP\_ERR\_TIMESTAMP\_INVALID and timestamp range check - ([7ec9f02](https://github.com/uviespace/airs-compression/commit/7ec9f02e28387ea77072dcb0a2f009b74d5709ea))
- Add Golomb Zero encoding mode with bitstream writer and encoder - ([61df88f](https://github.com/uviespace/airs-compression/commit/61df88f9b5b862a0b0ac38b5f2b66a89bf06897a))
- Feat (cmp): Use generic error for null parameters

- Replace CMP\_ERR\_PARAMS\_INVALID with CMP\_ERR\_GENERIC for null parameter checks
- Update corresponding tests - ([59e0781](https://github.com/uviespace/airs-compression/commit/59e078116216d1528b3da9fa17fdf4c0783d27a9))

### 🐛 Bug Fixes

- *(bitstream)* Zero-initialize bitstream\_writer and harden tests - ([375753e](https://github.com/uviespace/airs-compression/commit/375753e324433e80c1e34121d5ad419bdf814b77))
- *(cmp)* Improve cmp\_compress\_bound(), add proper header size field validation - ([e0efae7](https://github.com/uviespace/airs-compression/commit/e0efae732178975f590c9dd4ad3d69b16a9d8c3e))
- *(cmp)* Correct validation order in cmp\_initialise - ([b105deb](https://github.com/uviespace/airs-compression/commit/b105deb917839b5f4526e9acd231b162101fdcb8))
- *(cmp)* Check for error codes passed as size parameters - ([2827f1e](https://github.com/uviespace/airs-compression/commit/2827f1ed0ebeb6131bb250871b7b587afbadbeff))
- *(examples/simple\_compression)* Improve error handling, simplify, and clarify - ([f1d20cd](https://github.com/uviespace/airs-compression/commit/f1d20cdb7eb89043f5d426465aec153bbeb2acff))
- *(header)* Validate identifier and update timestamp error code - ([580273d](https://github.com/uviespace/airs-compression/commit/580273d7cb11170f134b8b126c03a4c47449dc7f))
- Improve build and example - ([92d6e29](https://github.com/uviespace/airs-compression/commit/92d6e293c69b6a6e36ab289b3f7b8b895e26a74c))
- Correct minor typos and inconsistencies - ([f4df9d9](https://github.com/uviespace/airs-compression/commit/f4df9d96807b8647149b52597902753aa625eb05))
- Add explicit int16\_t casts in diff/model preprocessors - ([fcc9495](https://github.com/uviespace/airs-compression/commit/fcc9495f7978e32473566130f9fdeeff9d4d60c0))
- Enforce buffer alignment and add error handling for unaligned buffers - ([d707a97](https://github.com/uviespace/airs-compression/commit/d707a9708fe985488aca7cf69d1f0a4c0d860172))
- Only set header fields when needed - ([e8e21be](https://github.com/uviespace/airs-compression/commit/e8e21befa3d92d92be21693ac94a4605fc04b0ea))
- Some typos - ([1ba0133](https://github.com/uviespace/airs-compression/commit/1ba01335a494ec5f8d617164b52961eb460413c9))

### ✨ Refactor

- *(cmp)* Make encoder stateless - ([befcc30](https://github.com/uviespace/airs-compression/commit/befcc3061873ca99c4d9be9b5318f37396f958f1))
- *(cmp)* Reorganize error codes by functional groups - ([d34ea52](https://github.com/uviespace/airs-compression/commit/d34ea52e0657eedc01ddf40986a183207e5c4d0d))
- *(compress)* Consolidate model updates in cmp\_compress\_u16 - ([edb5bc7](https://github.com/uviespace/airs-compression/commit/edb5bc76e79318a805e9f0cc7307b732896f6909))
- *(header)* Eliminate unused code - ([3b8b3af](https://github.com/uviespace/airs-compression/commit/3b8b3af75125cd85073afd4f7f5fcb58e9e71483))
- *(header)* Restructure header format and rename parameters - ([df5bdc6](https://github.com/uviespace/airs-compression/commit/df5bdc689d7c6625a97b98a1308070067020d49d))
- *(header)* Cleanup version flag and ID deserialization bitmasking - ([c9de700](https://github.com/uviespace/airs-compression/commit/c9de700ecad9f7f99bdb16b4c5ee6449f7d7fd46))
- *(test)* Move common test utilities to separate compilation unit - ([5491648](https://github.com/uviespace/airs-compression/commit/549164842da42d6333df54d13daaa4a723794fcd))
- *(test)* Remove Unity parametrized tests for C89 compatibility - ([b6e60a7](https://github.com/uviespace/airs-compression/commit/b6e60a7c4fe2e5e627534083b28bc152fad0c461))
- Standardize code documentation and style - ([51e7019](https://github.com/uviespace/airs-compression/commit/51e70193a4fa5d54f012e32d6875924b352c6cf2))
- Move uncompressed context creation to helper function - ([7c4555c](https://github.com/uviespace/airs-compression/commit/7c4555c28bbccae259df11a3f941aff1025bab0d))
- Refactor(compress) bitstream handling and header serialization

- Replace custom header serialization with direct bitstream writer usage
- Rename and move bitstream\_write.h to common/bitstream\_writer.h for clarity
- Add 64-bit support with bitstream\_write64() and bitstream\_rewind()
- Replace magic constants with bit field size definitions (CMP\_HDR\_BITS\_*) - ([b2ab2e4](https://github.com/uviespace/airs-compression/commit/b2ab2e44f06ca2bdb6ceaf627dcadc0f5511593c))
- Split public and internal header definitions - ([31061ac](https://github.com/uviespace/airs-compression/commit/31061ac7de56fa514f8f16cf7322d9543c628883))

### 📚 Documentation

- Add Logo - ([94ad070](https://github.com/uviespace/airs-compression/commit/94ad070104276d6a3774d6dcaab37c1cdddfc269))

### 🎨 Styling

- *(examples)* Fix code formatting and spacing - ([01c3565](https://github.com/uviespace/airs-compression/commit/01c3565f97cef277c4686b862959ca78bfefbfbd))
- *(format)* Improve code readability with consistent formatting rules - ([503d3c2](https://github.com/uviespace/airs-compression/commit/503d3c22b40cf4af8ed432154a9bf932439b9fe6))
- Fix typos and minor formatting inconsistencies - ([c7506d5](https://github.com/uviespace/airs-compression/commit/c7506d58f693160ce75428dfd693ecab57f2e29e))

### 🧪 Testing

- Add error checking and fix buffer alignment - ([b376f7b](https://github.com/uviespace/airs-compression/commit/b376f7bcf7fdc69254c3bc16d762b1d3f39255af))

### ⚙️ Miscellaneous Tasks

- Move dot program detection and fix compiler flag scope - ([0da2be0](https://github.com/uviespace/airs-compression/commit/0da2be00a39bc79df7d6d75728659604dd9c55d1))
- Bump version to 0.3.0 - ([4fd4491](https://github.com/uviespace/airs-compression/commit/4fd449160520224d2078d7d9e213aab7ff5dd463))

## [0.2.0](https://github.com/uviespace/airs-compression/compare/v0.1.0..v0.2.0) - 2025-03-17

### 🚀 Features

- *(cli)* Add AIRSPACE CLI tool - ([b575657](https://github.com/uviespace/airs-compression/commit/b575657130188c3d8fe65e39bc30aadcccb7b129))
- *(cli)* Add function to read input from stdin - ([1253437](https://github.com/uviespace/airs-compression/commit/1253437018a64d38aff58797cfaeae7bd4b66075))
- *(cli)* Prevent overwriting existing files - ([229a645](https://github.com/uviespace/airs-compression/commit/229a645235f1bdd5134dcfc2ddf2a04d21c186e7))
- *(compression)* [**breaking**] Introduce new compression API - ([fa2b426](https://github.com/uviespace/airs-compression/commit/fa2b426faf0bdcac4f31f612dbfd297c1320983f))
- *(doc)* Update Installation Guide and README documentation - ([ed515fb](https://github.com/uviespace/airs-compression/commit/ed515fbfe6f769ba3fb0d342b463cfb1bed091a7))
- Set C89 standard as default and improve build configuration - ([3e65206](https://github.com/uviespace/airs-compression/commit/3e652065463e4056a5dc870b5e8446349dd83f7a))

### 🐛 Bug Fixes

- *(docs)* Correct file paths and update main README - ([c8e522c](https://github.com/uviespace/airs-compression/commit/c8e522cb14248d8c2308aec8d4ec95f61926f494))

### 🔭 Other

- Improve error handling and code consistency

- Refactored error checking functions
- Improve compiler flags handling
- Standardized error code format specifiers and added <inttypes.h> for portability
- Updated version release to 1.1.1 - ([c6fdd68](https://github.com/uviespace/airs-compression/commit/c6fdd68ca780749323b4f3249972408be0669010))
- Update README.adoc

Add a ⚠️ to CAUTION note - ([c1b4475](https://github.com/uviespace/airs-compression/commit/c1b44753fb1dd701bc752a781d6f494c45235dba))

### ✨ Refactor

- Refactor cmp library to enhance error handling and modularity

- Move error number function declarations from `cmp.h` to `cmp\_errors.h`
- Added `cmp\_errors.h` include in `multi\_pass\_compression.c` and `single\_pass\_compression.c` examples - ([de8cc93](https://github.com/uviespace/airs-compression/commit/de8cc938ca173d24ea1d28e17e0ae9a4b6424b70))
- Refactor and update meson.build files, fix includes, and add error tests

- Refactor include directories
- Added new test file `test/test\_cmp\_errors.c`
- Improved test names - ([1b64219](https://github.com/uviespace/airs-compression/commit/1b64219c8448dfd9b1fb8ee0f2f310734cd9568c))
- Update include directives for consistency - ([dce76d7](https://github.com/uviespace/airs-compression/commit/dce76d721b12da0c9150cc86cfb581f60a376000))

### 🎨 Styling

- *(cli)* Reformat code for consistent style and readability - ([aed46b0](https://github.com/uviespace/airs-compression/commit/aed46b007ccd69807b1706502686d09cabbea2d2))

### 🧪 Testing

- *(cli)* Add CLI tests - ([d63595d](https://github.com/uviespace/airs-compression/commit/d63595d46fbc5649804927c5734dd26d49cb7292))

### ⚙️ Miscellaneous Tasks

- *(meson)* Add Asciidoctor for HTML generation - ([0bf71e7](https://github.com/uviespace/airs-compression/commit/0bf71e70750c9cdee85fda5d34d4aa4d734a0031))
- Add .clang-format configuration for Linux Kernel Style - ([6a11590](https://github.com/uviespace/airs-compression/commit/6a115906048992b3e52dad7a680bcd46920050b2))

## [0.1.0]- 2025-02-04

### 🔭 Other

-  Initial commit

The one where it all started - ([98a70fd](https://github.com/uviespace/airs-compression/commit/98a70fd948c2c18e71b53abe8c1526959d60e348))
