---
name: finding-alpha-zero-api
description: Find AlphaZero API header files from GitHub or the local build directory.
---

# Find AlphaZero API Header Files

Header files for this specific Xiang Qi implementation are in
the [include](../../../include/) directory.

The general AlphaZero API interface header files can be found in <https://github.com/shuyangsun/alpha-zero-api>,
specifically in the `src/include/alpha-zero-api` directory. However, GitHub may
block automated scripts from accessing the repository. In that case, you can
first build the project (use the `building-with-cmake` skill if you do not know
how), then look in the `build/debug/_deps/alphazeroapi-src` directory. The
header files are likely in
`build/debug/_deps/alphazeroapi-src/src/include/alpha-zero-api`. When in doubt,
search the build directory for `augmenter.h`.
