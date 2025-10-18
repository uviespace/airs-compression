[![AIRSPACE Logo](docs/AIRSPACE_Logo.svg)](https://github.com/uviespace/airs-compression)

## [unreleased]

### 🚀 Features

- *(cli)* Implement compression parameter parsing - ([02de0ed](https://github.com/uviespace/airs-compression/commit/02de0ed8202631a2a189d071b909d40d77324929))
- Add BCC2 GR712RC cross-compile support - ([b9a36a3](https://github.com/uviespace/airs-compression/commit/b9a36a31f9b80a94205caaa1b2f7c56c689cd6a6))

### 🐛 Bug Fixes

- Improve compression error log formatting - ([519e02a](https://github.com/uviespace/airs-compression/commit/519e02a64a6cfef2832567c6855e213152decfd7))

### 🔭 Other

- Refactor compiler flags to handle unit tests - ([833324b](https://github.com/uviespace/airs-compression/commit/833324b1f49246425fa6f53c2501605ee2f1f0c0))
- Centralize _POSIX_C_SOURCE for Linux - ([13e02a9](https://github.com/uviespace/airs-compression/commit/13e02a906ed54c6fcf35cd5fb1ff538d3a6fabd0))

### 📚 Documentation

- Improve INSTALL.adoc readability and build setup descriptions - ([ffae2d9](https://github.com/uviespace/airs-compression/commit/ffae2d91c8f8bd0e718d12c574009262e651caba))
- Add git-cliff for automated changelog generation - ([6ab845a](https://github.com/uviespace/airs-compression/commit/6ab845aa8b2dba463c12d4830bc7cbfea952ec96))
- Add initial CHANGELOG.md - ([2e55885](https://github.com/uviespace/airs-compression/commit/2e55885237b7a3dc42c8b110d04717b8c883b93a))

### ⚡ Performance

- *(bitstream)* Optimize put_be64 with aligned writes - ([4657b89](https://github.com/uviespace/airs-compression/commit/4657b89882be4a171d29d50d5fad65b21251600f))
- *(cmp)* Convert bitstream and encoder to sticky error handling - ([f91bb8b](https://github.com/uviespace/airs-compression/commit/f91bb8b6cfa0d9db3ced098096c08409d7a59ab7))
- *(encoder)* Optimize Golomb zero-escape sequence write - ([3951def](https://github.com/uviespace/airs-compression/commit/3951def65f12dc99acf6ff59fcca3d3235672e9e))

### 🎨 Styling

- Update and apply consistent code formatting with clang-format - ([b41974a](https://github.com/uviespace/airs-compression/commit/b41974ac27f9b671811e56fde7845bac34820aa3))

### 🧪 Testing

- Fix false positive leak sanitizer on Appel silicon - ([015fde6](https://github.com/uviespace/airs-compression/commit/015fde604c3455f3eb4b9c252f16f15e463048c6))

### ⚙️ Miscellaneous Tasks

- Fix typos - ([6842819](https://github.com/uviespace/airs-compression/commit/6842819cbe20bc3a30f2e35b9dc14077f7a0dfe7))
- Automate releases with GitHub Actions - ([8514f16](https://github.com/uviespace/airs-compression/commit/8514f16402f1cc22651198cdb96a47995af4304b))
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
- *(errors)* Add CMP_ERR_TIMESTAMP_INVALID and timestamp range check - ([7ec9f02](https://github.com/uviespace/airs-compression/commit/7ec9f02e28387ea77072dcb0a2f009b74d5709ea))
- Add Golomb Zero encoding mode with bitstream writer and encoder - ([61df88f](https://github.com/uviespace/airs-compression/commit/61df88f9b5b862a0b0ac38b5f2b66a89bf06897a))

### 🐛 Bug Fixes

- *(bitstream)* Zero-initialize bitstream_writer and harden tests - ([375753e](https://github.com/uviespace/airs-compression/commit/375753e324433e80c1e34121d5ad419bdf814b77))
- *(cmp)* Improve cmp_compress_bound(), add proper header size field validation - ([e0efae7](https://github.com/uviespace/airs-compression/commit/e0efae732178975f590c9dd4ad3d69b16a9d8c3e))
- *(cmp)* Correct validation order in cmp_initialise - ([b105deb](https://github.com/uviespace/airs-compression/commit/b105deb917839b5f4526e9acd231b162101fdcb8))
- *(cmp)* Check for error codes passed as size parameters - ([2827f1e](https://github.com/uviespace/airs-compression/commit/2827f1ed0ebeb6131bb250871b7b587afbadbeff))
- *(examples/simple_compression)* Improve error handling, simplify, and clarify - ([f1d20cd](https://github.com/uviespace/airs-compression/commit/f1d20cdb7eb89043f5d426465aec153bbeb2acff))
- *(header)* Validate identifier and update timestamp error code - ([580273d](https://github.com/uviespace/airs-compression/commit/580273d7cb11170f134b8b126c03a4c47449dc7f))
- Correct minor typos and inconsistencies - ([f4df9d9](https://github.com/uviespace/airs-compression/commit/f4df9d96807b8647149b52597902753aa625eb05))
- Add explicit int16_t casts in diff/model preprocessors - ([fcc9495](https://github.com/uviespace/airs-compression/commit/fcc9495f7978e32473566130f9fdeeff9d4d60c0))
- Enforce buffer alignment and add error handling for unaligned buffers - ([d707a97](https://github.com/uviespace/airs-compression/commit/d707a9708fe985488aca7cf69d1f0a4c0d860172))
- Only set header fields when needed - ([e8e21be](https://github.com/uviespace/airs-compression/commit/e8e21befa3d92d92be21693ac94a4605fc04b0ea))
- Some typos - ([1ba0133](https://github.com/uviespace/airs-compression/commit/1ba01335a494ec5f8d617164b52961eb460413c9))

### 🔭 Other

- Improve build and example - ([92d6e29](https://github.com/uviespace/airs-compression/commit/92d6e293c69b6a6e36ab289b3f7b8b895e26a74c))
- Move dot program detection and fix compiler flag scope - ([0da2be0](https://github.com/uviespace/airs-compression/commit/0da2be00a39bc79df7d6d75728659604dd9c55d1))
- Split public and internal header definitions - ([31061ac](https://github.com/uviespace/airs-compression/commit/31061ac7de56fa514f8f16cf7322d9543c628883))

### ✨ Refactor

- *(cmp)* Make encoder stateless - ([befcc30](https://github.com/uviespace/airs-compression/commit/befcc3061873ca99c4d9be9b5318f37396f958f1))
- *(cmp)* Reorganize error codes by functional groups - ([d34ea52](https://github.com/uviespace/airs-compression/commit/d34ea52e0657eedc01ddf40986a183207e5c4d0d))
- *(compress)* Consolidate model updates in cmp_compress_u16 - ([edb5bc7](https://github.com/uviespace/airs-compression/commit/edb5bc76e79318a805e9f0cc7307b732896f6909))
- *(header)* Eliminate unused code - ([3b8b3af](https://github.com/uviespace/airs-compression/commit/3b8b3af75125cd85073afd4f7f5fcb58e9e71483))
- *(header)* Restructure header format and rename parameters - ([df5bdc6](https://github.com/uviespace/airs-compression/commit/df5bdc689d7c6625a97b98a1308070067020d49d))
- *(header)* Cleanup version flag and ID deserialization bitmasking - ([c9de700](https://github.com/uviespace/airs-compression/commit/c9de700ecad9f7f99bdb16b4c5ee6449f7d7fd46))
- *(test)* Move common test utilities to separate compilation unit - ([5491648](https://github.com/uviespace/airs-compression/commit/549164842da42d6333df54d13daaa4a723794fcd))
- *(test)* Remove Unity parametrized tests for C89 compatibility - ([b6e60a7](https://github.com/uviespace/airs-compression/commit/b6e60a7c4fe2e5e627534083b28bc152fad0c461))
- Standardize code documentation and style - ([51e7019](https://github.com/uviespace/airs-compression/commit/51e70193a4fa5d54f012e32d6875924b352c6cf2))
- Move uncompressed context creation to helper function - ([7c4555c](https://github.com/uviespace/airs-compression/commit/7c4555c28bbccae259df11a3f941aff1025bab0d))

### 📚 Documentation

- Add Logo - ([94ad070](https://github.com/uviespace/airs-compression/commit/94ad070104276d6a3774d6dcaab37c1cdddfc269))

### 🎨 Styling

- *(examples)* Fix code formatting and spacing - ([01c3565](https://github.com/uviespace/airs-compression/commit/01c3565f97cef277c4686b862959ca78bfefbfbd))
- *(format)* Improve code readability with consistent formatting rules - ([503d3c2](https://github.com/uviespace/airs-compression/commit/503d3c22b40cf4af8ed432154a9bf932439b9fe6))
- Fix typos and minor formatting inconsistencies - ([c7506d5](https://github.com/uviespace/airs-compression/commit/c7506d58f693160ce75428dfd693ecab57f2e29e))

### 🧪 Testing

- Add error checking and fix buffer alignment - ([b376f7b](https://github.com/uviespace/airs-compression/commit/b376f7bcf7fdc69254c3bc16d762b1d3f39255af))

### ⚙️ Miscellaneous Tasks

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

- *(meson)* Add Asciidoctor for HTML generation - ([0bf71e7](https://github.com/uviespace/airs-compression/commit/0bf71e70750c9cdee85fda5d34d4aa4d734a0031))

### ✨ Refactor

- Update include directives for consistency - ([dce76d7](https://github.com/uviespace/airs-compression/commit/dce76d721b12da0c9150cc86cfb581f60a376000))

### 🎨 Styling

- *(cli)* Reformat code for consistent style and readability - ([aed46b0](https://github.com/uviespace/airs-compression/commit/aed46b007ccd69807b1706502686d09cabbea2d2))

### 🧪 Testing

- *(cli)* Add CLI tests - ([d63595d](https://github.com/uviespace/airs-compression/commit/d63595d46fbc5649804927c5734dd26d49cb7292))

### ⚙️ Miscellaneous Tasks

- Add .clang-format configuration for Linux Kernel Style - ([6a11590](https://github.com/uviespace/airs-compression/commit/6a115906048992b3e52dad7a680bcd46920050b2))
## [0.1.0] - 2025-02-04
