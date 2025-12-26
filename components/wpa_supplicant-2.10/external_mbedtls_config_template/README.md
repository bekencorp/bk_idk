# External mbedtls Integration Guide

This directory contains template files for integrating external mbedtls with armino's wpa_supplicant.

## Overview

armino's wpa_supplicant can use external mbedtls library instead of the built-in one. This is useful when:
- Your SDK already provides mbedtls (e.g., Tuya SDK, AWS IoT SDK)
- You need a specific mbedtls version
- You want to use a pre-built mbedtls library

## Configuration Options

Enable external mbedtls in your project's Kconfig:

```
CONFIG_USE_EXTERNAL_MBEDTLS=y
CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH="/path/to/your/mbedtls/config"
CONFIG_EXTERNAL_MBEDTLS_INCLUDE_PATH="/path/to/your/mbedtls/include"
CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y  # Set to y if using mbedtls 3.x
```

## Setup Steps

### 1. Copy Template Files

Copy the template files to your config directory:
```bash
mkdir -p /your/config/path/mbedtls
cp mbedtls/config.h /your/config/path/mbedtls/
cp mbedtls/arc4.h /your/config/path/mbedtls/  # Only for mbedtls 3.x
```

### 2. Modify config.h

Edit `config.h` to match your external mbedtls capabilities:

- **MBEDTLS_CMAC_C**: Uncomment if your mbedtls has `mbedtls_cipher_cmac_*` functions
- **MBEDTLS_NIST_KW_C**: Uncomment if your mbedtls has `mbedtls_nist_kw_*` functions
- **Version macros**: Update to match your mbedtls version

### 3. Set Kconfig Options

In your project config file:
```
CONFIG_USE_EXTERNAL_MBEDTLS=y
CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH="@WPA_CMAKE_ROOT@/../../your/config/path"
CONFIG_EXTERNAL_MBEDTLS_INCLUDE_PATH="@WPA_CMAKE_ROOT@/../../../../your/mbedtls/include"
CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y
```

Note: `@WPA_CMAKE_ROOT@` is a placeholder that gets expanded to `CMAKE_SOURCE_DIR` (armino directory).
Using `@...@` syntax avoids shell variable expansion issues with `${...}`.

Example for Tuya SDK:
```
CONFIG_USE_EXTERNAL_MBEDTLS=y
CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH="@WPA_CMAKE_ROOT@/../../tuyaos/tuyaos_adapter/include"
CONFIG_EXTERNAL_MBEDTLS_INCLUDE_PATH="@WPA_CMAKE_ROOT@/../../../../include/components/lib_tls/include"
CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y
```

### 4. Link Your mbedtls Library

Ensure your external mbedtls library (e.g., `libmbedtls.a`) is linked in your build.

## mbedtls 2.x vs 3.x Compatibility

### API Changes in mbedtls 3.x

| Function | mbedtls 2.x | mbedtls 3.x |
|----------|-------------|-------------|
| `mbedtls_pk_check_pair` | 2 params | 4 params (+ f_rng, p_rng) |
| `mbedtls_pk_parse_key` | 5 params | 7 params (+ f_rng, p_rng) |
| ECDH context | New structure | Use `MBEDTLS_ECDH_LEGACY_CONTEXT` |
| ARC4 | Available | Removed |

### Required Settings for mbedtls 3.x

1. Enable `CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y`
2. Define `MBEDTLS_ECDH_LEGACY_CONTEXT` in config.h
3. Include `arc4.h` stub in your config directory

## Troubleshooting

### Link Errors: undefined reference to mbedtls_*

Your external mbedtls may not have all functions. Check:
- `mbedtls_cipher_cmac_*` - Comment out `MBEDTLS_CMAC_C` if missing
- `mbedtls_nist_kw_*` - Comment out `MBEDTLS_NIST_KW_C` if missing

wpa_supplicant will use its own implementations when these are disabled.

### Compile Errors: structure member access

Enable `MBEDTLS_ECDH_LEGACY_CONTEXT` for mbedtls 3.x compatibility.

### Compile Errors: arc4.h not found

Copy the `arc4.h` stub to your config directory (mbedtls 3.x removed ARC4).

