#!/usr/bin/env python3
"""Generate the graphics card tables in 3Dmotor/pciids from pci.ids.

    python3 scripts/port/gen-pciids.py            rewrite the four headers
    python3 scripts/port/gen-pciids.py --check    report what would change, write nothing
    python3 scripts/port/gen-pciids.py --stats    print per vendor counts and exit

Four headers come out of one pass over third_party/pciids/pci.ids:

    vendors.h   VENDOR_* PCI vendor ids
    gpus.h      the NGfx::EVideoCard enum
    cards.h     vendor plus device id to EVideoCard, what GetVideoCard looks up
    detect.h    EVideoCard to quality defaults, what AutoDetectVideoConfig reads

The engine identifies a card by exact vendor and device id in GfxRender.cpp's
GetVideoCard, so a card missing from cards.h falls back to VC_DEFAULT and
autodetect calls it "Unknown card". That is a quality default, not a failure to
run, but it is silent, and the table is the only thing standing between a new
GPU and the 2006 defaults.

WHAT IS KEPT, AND WHY IT IS NOT EVERYTHING

pci.ids lists every PCI device a vendor ever shipped, not just display
adapters. Intel's block alone is 4694 entries and is overwhelmingly chipsets;
NVIDIA's carries the whole MCP line of SMBus, IDE, SATA, Ethernet, AC'97, USB
and PCI bridge functions. None of that can ever match a Direct3D adapter, so
including it would be dead weight in a table walked linearly on every startup.

So an entry is kept when its name looks like a display adapter and does not look
like something else on the same silicon. Both halves are needed: modern GPUs
ship an audio function whose name also carries the marketing name, so
"Navi 21 HDMI/DP Audio" matches the first test and has to fail the second.

The inclusion list is deliberately wider than the marketing line for one
vendor. It was "GeForce" alone once, which quietly dropped every NVIDIA
professional part, since those are named GL or GLM and carry Quadro, RTX A or
Tesla instead: an RTX A1000 Laptop GPU, device 0x25b9, is in pci.ids and was not
in cards.h. Anything that is a real display adapter belongs here whether or not
it was sold to gamers.

TUNING IT

Edit GPU_NAMES to admit a class of card, NON_GPU_NAMES to reject a function that
shares a marketing name with one. Run with --stats first: that prints what each
vendor contributes and is the quickest way to see whether a change let in a few
hundred chipsets. Regenerating is reproducible, so a change shows up as a diff.
"""

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PCI_IDS = os.path.join(REPO, "third_party", "pciids", "pci.ids")
OUT_DIR = os.path.join(REPO, "Versions", "Temporary", "Engine", "Sources",
                       "3Dmotor", "pciids")

# The vendors worth walking at all. Board partners are here because pci.ids
# files a few whole cards under them rather than under the chip vendor, and an
# adapter reporting one of those ids is otherwise unidentifiable.
VENDOR_IDS = [0x1002, 0x10de, 0x1458, 0x1462, 0x148c, 0x1682, 0x1da2, 0x8086]

# Looks like a display adapter. Covers the marketing lines of all three chip
# vendors plus the bare "... Graphics Controller" that Intel names its
# integrated parts with.
GPU_NAMES = re.compile(
    r"geforce|quadro|\brtx\b|\bgtx\b|\bnvs\b|tesla|\briva\b|\btnt\b|vanta|"
    r"radeon|firepro|firegl|instinct|\bvega\b|\brx *\d|\bhd \d{4}\b|"
    r"iris|hd graphics|uhd graphics|\barc\b|\bxe\b|graphics",
    re.I)

# Something other than the display engine, including the functions that sit on
# a graphics card and inherit its marketing name.
NON_GPU_NAMES = re.compile(
    r"\b(smbus|ide|sata|serial ata|ethernet|audio|usb|bridge|memory controller|"
    r"host bridge|isa|lpc|raid|co-?processor|nvme|sd host|thunderbolt|wireless|"
    r"wi-fi|network|controller hub|root port|dma|watchdog|management engine|"
    r"sensor|gpio|spi|uart|i2c|scsi|firewire|modem|nvswitch|nvlink|crypto|"
    r"signal processing|power management|chipset|northbridge|southbridge|dram|"
    r"hdmi|displayport|\bdp\b)\b",
    re.I)

# The rows AutoDetectVideoConfig falls through to when the card is not in the
# table. Hand written, kept verbatim: they are keyed on hardware level rather
# than on a device id and there is nothing in pci.ids to generate them from.
FALLBACK_ROWS = [
    '\t{ NGfx::VC_DEFAULT, NGfx::HL_R300,    NGScene::CV_MED,    NGScene::CV_HIGH,  0, "Unknown card, ps.2.0 class hardware" },',
    '\t{ NGfx::VC_DEFAULT, NGfx::HL_RADEON2, NGScene::CV_HIGH,   NGScene::CV_HIGH,  0, "Unknown card, ps.1.4 class hardware" },',
    '\t{ NGfx::VC_DEFAULT, NGfx::HL_GFORCE3, NGScene::CV_HIGH,   NGScene::CV_HIGH,  0, "Unknown card, ps.1.1 class hardware" },',
    '\t{ NGfx::VC_DEFAULT, NGfx::HL_TNL_DEVICE, NGScene::CV_VHIGH, NGScene::CV_MED,1, "Unknown card, DX7 class hardware" }',
]

# Every generated card gets the same defaults. The engine's own benchmark
# adjusts from here, so the table decides a starting point rather than a verdict,
# and a card new enough to be missing from an older pci.ids is a card that can
# afford these.
DEFAULT_ROW = "HWLEVEL_ANY, NGScene::CV_LOW, NGScene::CV_HIGH,   1"


def identifier(name):
    """A C identifier from a pci.ids name, by the rule the existing tables use.

    Anything that is not alphanumeric or a space is dropped rather than
    translated, so "GN20-P0-R-K2" becomes GN20P0RK2 and "6700/6700" becomes
    67006700, then runs of space collapse to one underscore. Callers prefix
    VC_ or VENDOR_, which is also what keeps a name that starts with a digit,
    such as Intel's 82G965, a legal identifier.
    """
    cleaned = re.sub(r"[^0-9A-Za-z ]+", "", name)
    return "_".join(cleaned.split()).upper()


def parse_pci_ids(path):
    """Vendor id to (vendor name, [(device id, device name)]), in file order."""
    vendors = {}
    current = None
    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            head = re.match(r"^([0-9a-f]{4})\s+(.*?)\s*$", line)
            if head:
                current = int(head.group(1), 16)
                if current in VENDOR_IDS:
                    vendors[current] = (head.group(2), [])
                continue
            device = re.match(r"^\t([0-9a-f]{4})\s\s(.*?)\s*$", line)
            if device and current in vendors:
                vendors[current][1].append((device.group(1), device.group(2)))
    return vendors


def is_display_adapter(name):
    return bool(GPU_NAMES.search(name)) and not NON_GPU_NAMES.search(name)


def collect(vendors):
    """Per vendor, the kept devices, in ascending vendor then device id order."""
    kept = []
    for vendor_id in sorted(vendors):
        vendor_name, devices = vendors[vendor_id]
        chosen = [(d, n) for d, n in devices if is_display_adapter(n)]
        chosen.sort(key=lambda dn: int(dn[0], 16))
        if chosen:
            kept.append((vendor_id, vendor_name, chosen))
    return kept


def write(path, lines, check):
    """Write with CRLF, as the existing headers use. True when it changed."""
    text = "\r\n".join(lines) + "\r\n"
    new = text.encode("utf-8")
    old = None
    if os.path.exists(path):
        with open(path, "rb") as handle:
            old = handle.read()
    if old == new:
        return False
    if not check:
        with open(path, "wb") as handle:
            handle.write(new)
    return True


def generate(kept, check):
    changed = []

    vendors_h = ["#pragma once", ""]
    for vendor_id, vendor_name, _ in kept:
        vendors_h.append("#define VENDOR_%s 0x%04x" % (identifier(vendor_name), vendor_id))
    if write(os.path.join(OUT_DIR, "vendors.h"), vendors_h, check):
        changed.append("vendors.h")

    # One enumerator per distinct name. Several device ids share a name, and the
    # enum needs each once while cards.h repeats it per id.
    gpus_h = ["#pragma once", "", "namespace NGfx", "{", "enum EVideoCard", "{",
              "\tVC_DEFAULT,"]
    seen = set()
    order = []
    for vendor_id, vendor_name, devices in kept:
        for _, device_name in devices:
            symbol = "VC_" + identifier(device_name)
            if symbol not in seen:
                seen.add(symbol)
                gpus_h.append("\t%s," % symbol)
                order.append((symbol, device_name))
        # The marker closes the vendor's block, which is where it sits today.
        gpus_h.append("\t////%s" % vendor_name)
    gpus_h += ["};", "}"]
    if write(os.path.join(OUT_DIR, "gpus.h"), gpus_h, check):
        changed.append("gpus.h")

    cards_h = ["#pragma once", "", "struct SVideoCardType", "{",
               "\tuint32_t dwVendorID;", "\tuint32_t dwDeviceID;",
               "\tuint32_t dwDeviceIDMask;", "\tNGfx::EVideoCard eType;", "};",
               "static SVideoCardType videoCardsArray[] =", "{"]
    for vendor_id, vendor_name, devices in kept:
        for device_id, device_name in devices:
            cards_h.append("{ VENDOR_%s, 0x%s, 0xFFFF, NGfx::VC_%s }," %
                           (identifier(vendor_name), device_id, identifier(device_name)))
    cards_h.append("};")
    if write(os.path.join(OUT_DIR, "cards.h"), cards_h, check):
        changed.append("cards.h")

    detect_h = ["#pragma once", "", "#define HWLEVEL_ANY -1",
                "struct SAutoDetectCfg", "{", "\tint nCard;", "\tint nLevel;",
                "\t////", "\tint nSpeed;", "\tint nTexture;", "",
                "\tint n16bppMode;", "\t////", "\tconst char *pszName;", "};",
                "static SAutoDetectCfg configAutoDetect[] =", "{",
                "\t//// CARD\t\t\t\t\t\t\t\t\tHW Level\t\t Speed\t  Texture  16bpp mode"]
    # One row per enumerator rather than per device id. The lookup breaks at the
    # first match, so repeating a row for every id sharing a name only lengthened
    # the walk.
    for symbol, device_name in order:
        detect_h.append('\t{ NGfx::%s,\t\t\t%s, "%s" },' %
                        (symbol, DEFAULT_ROW, device_name.replace('"', '\\"')))
    detect_h.append("\t////")
    detect_h += FALLBACK_ROWS
    detect_h.append("};")
    if write(os.path.join(OUT_DIR, "detect.h"), detect_h, check):
        changed.append("detect.h")

    return changed, len(order)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="report what would change, write nothing")
    parser.add_argument("--stats", action="store_true",
                        help="print per vendor counts and exit")
    args = parser.parse_args()

    if not os.path.exists(PCI_IDS):
        sys.exit("pci.ids not found at %s\n"
                 "run: git submodule update --init third_party/pciids" % PCI_IDS)

    vendors = parse_pci_ids(PCI_IDS)
    kept = collect(vendors)

    if args.stats:
        total = 0
        for vendor_id, vendor_name, devices in kept:
            listed = len(vendors[vendor_id][1])
            total += len(devices)
            print("0x%04x %-40s %5d of %5d" % (vendor_id, vendor_name[:40],
                                               len(devices), listed))
        print("%-47s %5d kept" % ("total", total))
        return

    changed, enum_count = generate(kept, args.check)
    cards = sum(len(d) for _, _, d in kept)
    print("%d device ids, %d distinct cards" % (cards, enum_count))
    if args.check:
        print("would rewrite: %s" % (", ".join(changed) if changed else "nothing"))
        sys.exit(1 if changed else 0)
    print("rewrote: %s" % (", ".join(changed) if changed else "nothing"))


if __name__ == "__main__":
    main()
