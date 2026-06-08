# BKSecure Packaging Tool

Click-based CLI for Beken secure image generation, signing, and packing. After `pip install .`, invoke as `bksecure`; you may also run `python3 main.py` from the tool directory.

## Installation

1. Clone from GitLab:  
   https://gitlab.bekencorp.com/armino/customer/signify/bk_tools  
   (example branch: `debug_bk7236n`)
2. Install from the `bksecure/setup.py` directory:

```bash
pip install .
```

3. Verify:

```bash
bksecure --version
bksecure --help
```

## Typical workflows

Run commands in a working directory that contains `partitions.csv`, `bin.csv`, `pack.json`, `security.csv`, `ota.csv`, and related artifacts.

**One-shot pack (default configs, sign + pack):**

```bash
bksecure pack all [--debug]
```

**Step-by-step: sign then build download image:**

```bash
bksecure steps sign \
  --pubkey_pem root_ec256_pubkey.pem \
  --privkey_pem root_ec256_privkey.pem \
  --ota_type OVERWRITE \
  --security_counter 5 \
  [--bl2_version 1.0.0] [--app_version 0.0.1] \
  [--replace_key_en true] [--new_pubkey_pem ...] [--new_privkey_pem ...] \
  [--debug]

bksecure steps pack \
  --soc_type bk7236n \
  --flash_aes_type FIXED \
  --flash_aes_key <64_hex_chars> \
  --ota_type OVERWRITE \
  [--security_counter 5] \
  [--boot_ota] \
  [--debug]
```

**Pack from pack.json (optional keys / AES):**

```bash
bksecure pack json [--pack_json pack.json] [--img_sign_privkey ...] [--flash_aes_key ...] [--flash_crc_en] [--data_aes_key ...] [--debug]
```

End-to-end example: `examples/flash_all_app/run.py`, `examples/flash_all_app/run.sh`.

---

## Command overview

| Group | Subcommand | Description |
|-------|------------|-------------|
| *(top-level)* | `image_info` | Print image information |
| `gen` | `partition` | Generate partition headers from `partitions.csv` |
| | `ppc` | Generate `_ppc.h` from `ppc.csv` / `gpio_dev.csv` |
| | `mpc` | Generate `_mpc.h` from `mpc.csv` |
| | `security` | Generate `security.h` from `security.csv` |
| | `ota` | Generate `_ota.h` from `ota.csv` |
| | `otp` | Generate OTP map from `otp2.csv` |
| | `otp_efuse` | Generate `otp_efuse_config.json` |
| | `all` | Generate all security-related headers |
| `pack` | `compress` | Compress OTA binary (OVERWRITE) |
| | `insert_pk_hash` | Insert public-key hash into a binary |
| | `get_pk_hash` | Compute hash from PEM public key |
| | `json` | Pack according to `pack.json` |
| | `all` | Full pack using default configs |
| `steps` | `sign` | Sign binaries in the current directory |
| | `pack` | Encrypt and produce download binaries |
| | `pack_csv` | Pack using CSV-driven flow |
| | `get_app_bin_hash` / `sign_app_bin_hash` / `sign_from_app_sig` | Split flow: app image hash signing |
| | `get_ota_bin_hash` / `sign_ota_bin_hash` / `sign_from_ota_sig` | Split flow: OTA image hash signing |

Most subcommands accept `--debug` for verbose logging unless noted otherwise.

---

## `steps sign` options

| Option | Description |
|--------|-------------|
| `--pubkey_pem` | Image signing public key PEM (default: `img_sign_pubkey`) |
| `--privkey_pem` | Image signing private key PEM (default: `img_sign_privkey`) |
| `--ota_type` | OTA strategy: `XIP` \| `OVERWRITE` |
| `--bl2_version` | Override BL2 version (optional) |
| `--app_version` | Override APP version (optional) |
| `--security_counter` | Security counter (integer, optional) |
| `--replace_key_en` | Key replacement enable (string, default `False`; parsed as true/false/none) |
| `--new_pubkey_pem` | New public key PEM when replacing keys |
| `--new_privkey_pem` | New private key PEM when replacing keys |
| `--debug` | Debug logging |

## `steps pack` options

| Option | Description |
|--------|-------------|
| `--soc_type` | SoC: `bk7236` \| `bk7236n` (default `bk7236n`) |
| `--flash_aes_type` | **Required** Flash AES: `NONE` \| `FIXED` \| `RANDOM` |
| `--flash_aes_key` | Flash AES key (`FIXED` requires a value) |
| `--ota_type` | OTA strategy: `XIP` \| `OVERWRITE` |
| `--security_counter` | Security counter (optional) |
| `--boot_ota` | Enable bootloader OTA (flag) |
| `--debug` | Debug logging |

## `gen otp_efuse` options (summary)

| Option | Description |
|--------|-------------|
| `--flash_aes_type` | **Required** `FIXED` \| `RANDOM` \| `NONE` |
| `--flash_crc_en` | Flash CRC: `True` / `False` |
| `--flash_aes_key` | AES key |
| `--pubkey_pem_file` | Secure-boot public key PEM (default `root_ec256_pubkey.pem`) |
| `--secure_boot` | Enable secure boot (flag) |
| `--outfile` | Output JSON (default `otp_efuse_config.json`) |

## `gen partition` options (summary)

| Option | Description |
|--------|-------------|
| `--partition_csv` | Partition CSV (default `partitions.csv`) |
| `--ota_type` | **Required** `OVERWRITE` \| `XIP` |
| `--out_hdr_file` / `--out_layout_file` | Output header paths |

For full option lists:

```bash
bksecure <group> <command> --help
# e.g.
bksecure steps pack --help
bksecure gen otp_efuse --help
```
