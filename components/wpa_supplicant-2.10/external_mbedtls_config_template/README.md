# External mbedtls Integration Guide

armino's wpa_supplicant can use external mbedtls library instead of the built-in one. This is useful when:
- Your SDK already provides mbedtls (e.g., Tuya SDK, AWS IoT SDK)
- You need a specific mbedtls version
- You want to use a pre-built mbedtls library

## Configuration Options

Enable external mbedtls in your project's config file:

```
CONFIG_USE_EXTERNAL_MBEDTLS=y
CONFIG_SDK_ROOT="@CMAKE_SOURCE_DIR@/../../../.."
CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH="@SDK@/path/to/config.h"
CONFIG_EXTERNAL_MBEDTLS_INCLUDE_PATH="@SDK@/path/to/include"
CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y  # Set to y if using mbedtls 3.x
```

## Path Placeholders

| Placeholder | Replaced with | Description |
|-------------|---------------|-------------|
| `@CMAKE_SOURCE_DIR@` | CMake's CMAKE_SOURCE_DIR | armino directory (vendor/T7/t7_os/armino) |
| `@SDK@` | CONFIG_SDK_ROOT value | SDK root directory |

**Resolution order:**
1. `@CMAKE_SOURCE_DIR@` in CONFIG_SDK_ROOT -> actual cmake source path
2. `@SDK@` in other paths -> resolved CONFIG_SDK_ROOT value

## Example: Tuya SDK (bk7236_tuya_sdk)

```
# SDK_ROOT = bk7236_tuya_sdk = @CMAKE_SOURCE_DIR@/../../../..
CONFIG_USE_EXTERNAL_MBEDTLS=y
CONFIG_SDK_ROOT="@CMAKE_SOURCE_DIR@/../../../.."
CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH="@SDK@/include/components/svc_tuya_cloud/include/tls/tuya_tls_config.h"
CONFIG_EXTERNAL_MBEDTLS_INCLUDE_PATH="@SDK@/include/components/lib_tls/include;@SDK@/include/base/include;@SDK@/include/components/svc_tuya_cloud/include/tls;@SDK@/include/components/tal_system/include;@SDK@/vendor/T7/tuyaos/tuyaos_adapter/include/utilities/include"
CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y
```

## mbedtls 2.x vs 3.x Compatibility

### API Changes in mbedtls 3.x

| Function | mbedtls 2.x | mbedtls 3.x |
|----------|-------------|-------------|
| `mbedtls_pk_check_pair` | 2 params | 4 params (+ f_rng, p_rng) |
| `mbedtls_pk_parse_key` | 5 params | 7 params (+ f_rng, p_rng) |
| ECDH context | New structure | Use `MBEDTLS_ECDH_LEGACY_CONTEXT` |
| ARC4 | Available | Removed |

### Required Settings for mbedtls 3.x

Enable `CONFIG_EXTERNAL_MBEDTLS_VERSION_3X=y` in your config file.

## Troubleshooting

### Link Errors: undefined reference to mbedtls_*

Your external mbedtls may not have all functions. wpa_supplicant will use its own implementations when certain features are disabled.

### Compile Errors: structure member access

Ensure `MBEDTLS_ECDH_LEGACY_CONTEXT` is defined in your mbedtls config for 3.x compatibility.
