#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
from dataclasses import dataclass
from typing import Dict, List, Tuple

sys.dont_write_bytecode = True


@dataclass
class SpidEntry:
    vendor_name: str
    spid_value_raw: str
    spid_value_int: int
    spid_value_len: int
    support_buck: bool
    support_default_spid: bool


@dataclass
class ResolvedSpid:
    requested_vendor_name: str
    selected_entry: SpidEntry
    fallback_used: bool


@dataclass
class ResolvedVendor:
    requested_vendor_name: str
    vendor_name: str
    entries: List[SpidEntry]
    fallback_used: bool


def _normalize_soc_name(soc_name: str) -> str:
    if soc_name is None:
        raise ValueError("SOC name is required")
    normalized = soc_name.strip().upper()
    if not normalized:
        raise ValueError("SOC name is empty")
    return normalized


def _read_first_value(file_path: str) -> str:
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"SPID config file not found: {file_path}")

    with open(file_path, "r", encoding="utf-8") as file:
        for line in file:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            return stripped

    raise ValueError(f"No valid vendor name found in {file_path}")


def _parse_bool_yn(raw_value: str, field_name: str) -> bool:
    normalized = raw_value.strip().lower()
    if normalized == "y":
        return True
    if normalized == "n":
        return False
    raise ValueError(f"Invalid {field_name}='{raw_value}', expected y/n")


def parse_spid_value(spid_value: str) -> Tuple[int, int]:
    value = spid_value.strip()
    if not value:
        raise ValueError("spid_value is empty")

    if value == "0":
        return 0, 0

    if value.lower().startswith("0x"):
        hex_part = value[2:]
        if not hex_part:
            raise ValueError(f"Invalid hex SPID value: {value}")
        if len(hex_part) > 32:
            raise ValueError(f"Hex SPID too long ({len(hex_part) // 2} bytes): {value}")
        if len(hex_part) % 2 != 0:
            raise ValueError(f"Hex SPID length must be even: {value}")
        return int(hex_part, 16), len(hex_part) // 2

    if len(value) > 15:
        raise ValueError(f"ASCII SPID too long ({len(value)} chars): {value}")

    spid_int = 0
    for char in value:
        spid_int = (spid_int << 8) | ord(char)
    return spid_int, len(value)


def spid_int_to_bytes(spid_value_int: int, spid_value_len: int, max_bytes: int = 16) -> List[int]:
    if spid_value_len < 0 or spid_value_len > max_bytes:
        raise ValueError(f"Invalid spid_value_len={spid_value_len}")

    output = []
    for index in range(spid_value_len):
        shift = (spid_value_len - 1 - index) * 8
        output.append((spid_value_int >> shift) & 0xFF)

    while len(output) < max_bytes:
        output.append(0x00)
    return output


def _parse_spid_csv_line(line: str, line_number: int, file_path: str) -> SpidEntry:
    columns = [column.strip() for column in line.split(",")]
    if len(columns) != 4:
        raise ValueError(
            f"{file_path}:{line_number} invalid column count {len(columns)}, expected 4: "
            "vendor_name,spid_value,support_buck,support_default_spid"
        )

    vendor_name, spid_value_raw, support_buck_raw, support_default_raw = columns
    if not vendor_name:
        raise ValueError(f"{file_path}:{line_number} vendor_name is empty")

    spid_value_int, spid_value_len = parse_spid_value(spid_value_raw)
    support_buck = _parse_bool_yn(support_buck_raw, "support_buck")
    support_default_spid = _parse_bool_yn(support_default_raw, "support_default_spid")

    return SpidEntry(
        vendor_name=vendor_name,
        spid_value_raw=spid_value_raw,
        spid_value_int=spid_value_int,
        spid_value_len=spid_value_len,
        support_buck=support_buck,
        support_default_spid=support_default_spid,
    )


def load_spid_list(spid_list_path: str, soc_name: str) -> Dict[str, List[SpidEntry]]:
    if not os.path.exists(spid_list_path):
        raise FileNotFoundError(f"SPID list file not found: {spid_list_path}")

    entries_by_vendor: Dict[str, List[SpidEntry]] = {}
    unique_spid_values = set()

    with open(spid_list_path, "r", encoding="utf-8") as file:
        for line_number, raw_line in enumerate(file, 1):
            stripped = raw_line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            entry = _parse_spid_csv_line(stripped, line_number, spid_list_path)

            spid_key = (entry.spid_value_int, entry.spid_value_len)
            if spid_key in unique_spid_values:
                raise ValueError(
                    f"Duplicate spid_value '{entry.spid_value_raw}' in {spid_list_path}"
                )
            unique_spid_values.add(spid_key)
            entries_by_vendor.setdefault(entry.vendor_name, []).append(entry)

    if not entries_by_vendor:
        raise ValueError(f"No valid entries found in {spid_list_path}")

    _validate_soc_default_entries(entries_by_vendor, _normalize_soc_name(soc_name), spid_list_path)
    return entries_by_vendor


def _validate_soc_default_entries(
    entries_by_vendor: Dict[str, List[SpidEntry]], soc_name_upper: str, spid_list_path: str
) -> None:
    _ = soc_name_upper
    beken_entries = entries_by_vendor.get("BEKEN", [])
    if not beken_entries:
        raise ValueError(f"{spid_list_path} must contain default vendor BEKEN")


def _select_entry_for_vendor(
    entries: List[SpidEntry],
    vendor_name: str,
    spid_list_path: str,
    prefer_buck: bool = False,
) -> SpidEntry:
    if len(entries) == 1:
        return entries[0]

    matched = [entry for entry in entries if entry.support_buck == prefer_buck]
    if len(matched) == 1:
        return matched[0]

    expected = "support_buck=y" if prefer_buck else "support_buck=n"
    raise ValueError(
        f"{spid_list_path} vendor '{vendor_name}' has ambiguous entries, cannot select {expected}"
    )


def get_spid_list_path(repo_root: str, soc_name: str) -> str:
    return os.path.join(repo_root, "properties", "soc", soc_name.lower(), "spid_list.csv")


def _vendor_exists_in_any_soc(spid_list_path: str, vendor_name: str) -> bool:
    # spid_list_path: <repo>/properties/soc/<soc>/spid_list.csv
    # scan sibling SOC directories under <repo>/properties/soc
    soc_root = os.path.dirname(os.path.dirname(spid_list_path))
    if not os.path.isdir(soc_root):
        return False

    for entry in os.listdir(soc_root):
        candidate_csv = os.path.join(soc_root, entry, "spid_list.csv")
        if not os.path.isfile(candidate_csv):
            continue
        try:
            entries_by_vendor = load_spid_list(candidate_csv, entry.upper())
        except Exception:
            # ignore unrelated/invalid SOC lists while checking global existence
            continue
        if vendor_name in entries_by_vendor:
            return True
    return False


def resolve_spid(
    soc_name: str,
    spid_txt_path: str,
    spid_list_path: str,
    prefer_buck: bool = False,
) -> ResolvedSpid:
    requested_vendor_name = _read_first_value(spid_txt_path)
    soc_name_upper = _normalize_soc_name(soc_name)
    entries_by_vendor = load_spid_list(spid_list_path, soc_name_upper)

    if requested_vendor_name in entries_by_vendor:
        selected = _select_entry_for_vendor(
            entries_by_vendor[requested_vendor_name],
            requested_vendor_name,
            spid_list_path,
            prefer_buck=prefer_buck,
        )
        return ResolvedSpid(
            requested_vendor_name=requested_vendor_name,
            selected_entry=selected,
            fallback_used=False,
        )

    # If vendor is not defined in any SOC list, treat as configuration error.
    if not _vendor_exists_in_any_soc(spid_list_path, requested_vendor_name):
        raise ValueError(
            f"Vendor '{requested_vendor_name}' is not defined in any SOC spid_list.csv, abort build"
        )

    # Fallback: vendor exists globally but undefined for current SOC -> BEKEN non-buck default
    beken_entries = entries_by_vendor.get("BEKEN", [])
    if not beken_entries:
        raise ValueError(
            f"Vendor '{requested_vendor_name}' not found and BEKEN default is missing for SOC {soc_name_upper}"
        )
    fallback_entry = _select_entry_for_vendor(
        beken_entries,
        "BEKEN",
        spid_list_path,
        prefer_buck=False,
    )

    return ResolvedSpid(
        requested_vendor_name=requested_vendor_name,
        selected_entry=fallback_entry,
        fallback_used=True,
    )


def resolve_vendor_entries(
    soc_name: str,
    spid_txt_path: str,
    spid_list_path: str,
) -> ResolvedVendor:
    """Resolve all spid_list entries that belong to the requested vendor.

    Unlike resolve_spid(), this returns every entry of the vendor so the
    firmware can match the real OTP at runtime (longest-prefix) instead of
    baking a single build-time choice. Fallback semantics stay the same:
      - vendor missing in every SOC list  -> abort build
      - vendor missing only for this SOC   -> use BEKEN entries of this SOC
    """
    requested_vendor_name = _read_first_value(spid_txt_path)
    soc_name_upper = _normalize_soc_name(soc_name)
    entries_by_vendor = load_spid_list(spid_list_path, soc_name_upper)

    if requested_vendor_name in entries_by_vendor:
        return ResolvedVendor(
            requested_vendor_name=requested_vendor_name,
            vendor_name=requested_vendor_name,
            entries=entries_by_vendor[requested_vendor_name],
            fallback_used=False,
        )

    if not _vendor_exists_in_any_soc(spid_list_path, requested_vendor_name):
        raise ValueError(
            f"Vendor '{requested_vendor_name}' is not defined in any SOC spid_list.csv, abort build"
        )

    beken_entries = entries_by_vendor.get("BEKEN", [])
    if not beken_entries:
        raise ValueError(
            f"Vendor '{requested_vendor_name}' not found and BEKEN default is missing for SOC {soc_name_upper}"
        )

    return ResolvedVendor(
        requested_vendor_name=requested_vendor_name,
        vendor_name="BEKEN",
        entries=beken_entries,
        fallback_used=True,
    )
