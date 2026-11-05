#!/usr/bin/env python3
"""Build a comparison page from an IconRender run.

IconRender (Versions/Temporary/Engine/Sources/IconRender) writes, per unit,
the shipped icon (orig.dds), the model editor's shot reproduced exactly
(x1.tga) and a high resolution render (hq.tga), with a manifest.tsv listing
them. This page puts three pictures side by side for every unit:

    original           the shipped 42x42 icon, magnified without smoothing
    high quality       hq.tga at its own size
    high quality, 42   hq.tga reduced back to 42x42, magnified like the original

and scores how close the re-render is to the shipped picture, both for the
exact reproduction (x1) and for the reduced high quality one. A low score
points at a unit whose scene differs from what the editor had: another light,
another model, a camera that was never saved. Rows can be sorted by score.

Pictures are compared composited over the editor's background grey, so a
difference in alpha counts where it is visible. SSIM is on luminance with a
7x7 box window, which is what a 42 pixel image can carry.

    python scripts/iconrender/icon_report.py C:/Games/bk2/IconRender
    python scripts/iconrender/icon_report.py OUT --summary   # scores only, no page
    python scripts/iconrender/icon_report.py OUT --standalone vanilla.html

The page goes to OUT/report/index.html, the PNGs it shows beside it.

--standalone writes a page with every picture inside it instead, to send as one
file: WebP in data: URIs (the 4x render lossy at --quality, the shipped icon
lossless), and the reduced 42 pixel column worked out by the page itself from
the 4x one, with the same box filter. A page over --max-mb (Discord's upload
limit by default) is split into numbered parts by side and category, each
linking to the others, which work once they sit in the same folder.
"""
import argparse
import base64
import io
import csv
import html
import json
import os
import sys

import numpy as np
from PIL import Image

ICON = 42
BACKGROUND = 128


def load_rgba(path):
    """An RGBA float array, or None when the file is missing or unreadable."""
    if not os.path.exists(path):
        return None
    try:
        with Image.open(path) as im:
            return np.asarray(im.convert("RGBA"), dtype=np.float64)
    except OSError:
        return None


def reduce(rgba, size):
    """Box filter to size x size with premultiplied alpha, as IconRender does."""
    h, w = rgba.shape[:2]
    if h == size and w == size:
        return rgba
    f = h // size
    a = rgba[..., 3:4]
    pre = np.concatenate([rgba[..., :3] * a, a], axis=2)
    pre = pre[: size * f, : size * f].reshape(size, f, size, f, 4).mean(axis=(1, 3))
    out = np.empty_like(pre)
    alpha = pre[..., 3:4]
    with np.errstate(invalid="ignore", divide="ignore"):
        out[..., :3] = np.where(alpha > 0, pre[..., :3] / alpha, BACKGROUND)
    out[..., 3:4] = alpha
    return out


def composite(rgba):
    a = rgba[..., 3:4] / 255.0
    return rgba[..., :3] * a + BACKGROUND * (1.0 - a)


def psnr(a, b):
    mse = np.mean((a - b) ** 2)
    if mse == 0:
        return float("inf")
    return 10.0 * np.log10(255.0 ** 2 / mse)


def box(x, k=7):
    """Mean over a k x k window, valid region only."""
    c = np.cumsum(np.cumsum(np.pad(x, ((1, 0), (1, 0))), axis=0), axis=1)
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)


def ssim(a, b):
    la = a @ [0.299, 0.587, 0.114]
    lb = b @ [0.299, 0.587, 0.114]
    c1, c2 = (0.01 * 255) ** 2, (0.03 * 255) ** 2
    ma, mb = box(la), box(lb)
    va = box(la * la) - ma * ma
    vb = box(lb * lb) - mb * mb
    cov = box(la * lb) - ma * mb
    s = ((2 * ma * mb + c1) * (2 * cov + c2)) / ((ma * ma + mb * mb + c1) * (va + vb + c2))
    return float(s.mean())


def scores(ref, test):
    """PSNR and SSIM of test against ref, both 42x42; None if either is missing."""
    if ref is None or test is None or ref.shape != test.shape:
        return None
    a, b = composite(ref), composite(test)
    return {"psnr": psnr(a, b), "ssim": ssim(a, b)}


def save_png(rgba, path):
    Image.fromarray(np.clip(np.rint(rgba), 0, 255).astype(np.uint8), "RGBA").save(path)


def fmt_score(s):
    if s is None:
        return "-"
    p = "inf" if s["psnr"] == float("inf") else f"{s['psnr']:.1f}"
    return f"{p} dB / {s['ssim']:.3f}"


PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Icon Re-render</title>
<style>
:root {{ --bg: #f4f4f2; --fg: #1d1d1b; --muted: #6b6b66; --line: #d8d8d2; --cell: #808080; --bad: #b3261e; }}
@media (prefers-color-scheme: dark) {{
  :root {{ --bg: #1b1b1a; --fg: #ececea; --muted: #9a9a94; --line: #34342f; --bad: #f2b8b5; }}
}}
body {{ margin: 0; padding: 16px; background: var(--bg); color: var(--fg); font: 14px/1.4 system-ui, sans-serif; }}
h1 {{ font-size: 20px; margin: 0 0 4px; }}
.meta {{ color: var(--muted); margin-bottom: 12px; }}
.bar {{ display: flex; flex-wrap: wrap; gap: 8px 16px; align-items: center; margin-bottom: 12px; }}
input, select {{ font: inherit; padding: 4px 6px; }}
table {{ border-collapse: collapse; width: 100%; }}
th, td {{ border-bottom: 1px solid var(--line); padding: 6px 8px; text-align: left; vertical-align: middle; }}
th {{ position: sticky; top: 0; background: var(--bg); }}
td.pic {{ width: {view}px; }}
td.pic img {{ display: block; width: {view}px; height: {view}px; background: var(--cell); image-rendering: pixelated; }}
body.checker td.pic img {{ background: repeating-conic-gradient(#bbb 0 25%, #eee 0 50%) 0 0 / 16px 16px; }}
td.pic img.smooth {{ image-rendering: auto; }}
.name {{ font-weight: 600; }}
.id {{ color: var(--muted); font-size: 12px; word-break: break-all; }}
.score {{ font-variant-numeric: tabular-nums; white-space: nowrap; }}
.warn {{ color: var(--bad); }}
@media (max-width: 700px) {{ td.pic img {{ width: 84px; height: 84px; }} td.pic {{ width: 84px; }} }}
</style>
</head>
<body>
<h1>Unit icon re-render</h1>
<div class="meta">{count} units, light {light}, scale {scale}x. Scores are PSNR / SSIM against the shipped icon.</div>
<div class="bar">
  <label>Filter <input id="q" type="search" placeholder="name or folder"></label>
  <label>Sort <select id="sort">
    <option value="folder">folder</option>
    <option value="name">name</option>
    <option value="x1">exact reproduction, worst first</option>
    <option value="hq">reduced high quality, worst first</option>
  </select></label>
  <label><input id="checker" type="checkbox"> checkerboard behind transparency</label>
</div>
<table>
<thead><tr><th>Unit</th><th>Original</th><th>High quality</th><th>High quality, {icon}px</th><th>Scores</th></tr></thead>
<tbody id="rows"></tbody>
</table>
<script>
const rows = {rows};
const tbody = document.getElementById("rows");
function esc(s) {{ return String(s).replace(/[&<>"]/g, c => ({{"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}})[c]); }}
function pic(src, cls) {{ return src ? `<td class="pic"><a href="${{esc(src)}}"><img loading="lazy" class="${{cls}}" src="${{esc(src)}}" alt=""></a></td>` : '<td class="pic">-</td>'; }}
function render() {{
  const q = document.getElementById("q").value.toLowerCase();
  const key = document.getElementById("sort").value;
  let list = rows.filter(r => !q || (r.name + " " + r.folder).toLowerCase().includes(q));
  const num = v => v === null ? -1 : v;
  if (key === "name") list.sort((a, b) => a.name.localeCompare(b.name));
  else if (key === "x1") list.sort((a, b) => num(a.x1) - num(b.x1));
  else if (key === "hq") list.sort((a, b) => num(a.hq) - num(b.hq));
  else list.sort((a, b) => a.folder.localeCompare(b.folder));
  tbody.innerHTML = list.map(r => `<tr>
    <td><div class="name">${{esc(r.name || r.short)}}</div><div class="id">${{esc(r.short)}}</div><div class="id">${{esc(r.stats)}}</div>${{r.note ? `<div class="warn">${{esc(r.note)}}</div>` : ""}}</td>
    ${{pic(r.orig, "")}}${{pic(r.hqimg, "smooth")}}${{pic(r.small, "")}}
    <td class="score"><div>exact: ${{esc(r.x1txt)}}</div><div>reduced: ${{esc(r.hqtxt)}}</div>${{r.x1img ? `<div><a href="${{esc(r.x1img)}}">exact reproduction</a></div>` : ""}}</td>
  </tr>`).join("");
}}
for (const id of ["q", "sort"]) document.getElementById(id).addEventListener("input", render);
document.getElementById("checker").addEventListener("change", e => document.body.classList.toggle("checker", e.target.checked));
render();
</script>
</body>
</html>
"""


def webp_b64(rgba, lossless, quality):
    """A data: URI of the picture as WebP."""
    im = Image.fromarray(np.clip(np.rint(rgba), 0, 255).astype(np.uint8), "RGBA")
    buf = io.BytesIO()
    if lossless:
        im.save(buf, "WEBP", lossless=True, method=6)
    else:
        # exact keeps the colour under transparent pixels, which the page's box
        # filter weighs by alpha anyway, so it costs nothing to keep it right
        im.save(buf, "WEBP", quality=quality, method=6, exact=True)
    return "data:image/webp;base64," + base64.b64encode(buf.getvalue()).decode("ascii")


def group_of(folder):
    """Side and category, Units/Technics/German/Tanks/x/ -> German/Tanks."""
    parts = folder.strip("/").split("/")
    if len(parts) >= 4 and parts[0].lower() == "units" and parts[1].lower() == "technics":
        return parts[2] + "/" + parts[3]
    return "/".join(parts[:2]) if parts else "other"


def split_parts(rows, limit):
    """Rows, sorted by folder, cut into parts of at most limit bytes of pictures,
    a category kept whole unless it alone is over the limit."""
    rows = sorted(rows, key=lambda r: r["folder"].lower())
    groups = []
    for r in rows:
        g = group_of(r["folder"])
        if not groups or groups[-1][0].lower() != g.lower():
            groups.append((g, []))
        groups[-1][1].append(r)
    parts = [[]]
    size = 0
    for _, members in groups:
        weight = sum(r["bytes"] for r in members)
        if parts[-1] and size + weight > limit:
            parts.append([])
            size = 0
        for r in members:
            if parts[-1] and size + r["bytes"] > limit:
                parts.append([])
                size = 0
            parts[-1].append(r)
            size += r["bytes"]
    return parts


STANDALONE_PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<style>
:root {{ --bg: #f4f4f2; --fg: #1d1d1b; --muted: #6b6b66; --line: #d8d8d2; --cell: #808080; --bad: #b3261e; --link: #2557a7; }}
@media (prefers-color-scheme: dark) {{
  :root {{ --bg: #1b1b1a; --fg: #ececea; --muted: #9a9a94; --line: #34342f; --bad: #f2b8b5; --link: #9fc0f5; }}
}}
body {{ margin: 0; padding: 16px; background: var(--bg); color: var(--fg); font: 14px/1.4 system-ui, sans-serif; }}
a {{ color: var(--link); }}
h1 {{ font-size: 20px; margin: 0 0 4px; }}
.meta {{ color: var(--muted); margin-bottom: 8px; }}
.parts {{ margin-bottom: 12px; display: flex; flex-wrap: wrap; gap: 4px 12px; }}
.parts .here {{ font-weight: 600; }}
.bar {{ display: flex; flex-wrap: wrap; gap: 8px 16px; align-items: center; margin-bottom: 12px; }}
input, select {{ font: inherit; padding: 4px 6px; }}
table {{ border-collapse: collapse; width: 100%; }}
th, td {{ border-bottom: 1px solid var(--line); padding: 6px 8px; text-align: left; vertical-align: middle; }}
th {{ position: sticky; top: 0; background: var(--bg); z-index: 1; }}
td.pic {{ width: {view}px; }}
td.pic img, td.pic canvas {{ display: block; width: {view}px; height: {view}px; background: var(--cell); image-rendering: pixelated; }}
body.checker td.pic img, body.checker td.pic canvas {{ background: repeating-conic-gradient(#bbb 0 25%, #eee 0 50%) 0 0 / 16px 16px; }}
td.pic img.smooth {{ image-rendering: auto; }}
.name {{ font-weight: 600; }}
.id {{ color: var(--muted); font-size: 12px; word-break: break-all; }}
.score {{ font-variant-numeric: tabular-nums; white-space: nowrap; }}
.warn {{ color: var(--bad); }}
@media (max-width: 700px) {{ td.pic img, td.pic canvas {{ width: 84px; height: 84px; }} td.pic {{ width: 84px; }} }}
</style>
</head>
<body>
<h1>{heading}</h1>
<div class="meta">{count} units here{of_total}. Light {light}, scale {scale}x. Scores are PSNR / SSIM against the shipped icon:
"exact" is the model editor's shot reproduced, "reduced" the high quality picture brought back to {icon}px.</div>
{parts}
<div class="bar">
  <label>Filter <input id="q" type="search" placeholder="name or folder"></label>
  <label>Sort <select id="sort">
    <option value="folder">folder</option>
    <option value="name">name</option>
    <option value="x1">exact reproduction, worst first</option>
    <option value="hq">reduced high quality, worst first</option>
  </select></label>
  <label><input id="checker" type="checkbox"> checkerboard behind transparency</label>
</div>
<table>
<thead><tr><th>Unit</th><th>Original</th><th>High quality</th><th>High quality, {icon}px</th><th>Scores</th></tr></thead>
<tbody id="rows"></tbody>
</table>
<script>
const ICON = {icon};
const rows = {rows};
const tbody = document.getElementById("rows");
function esc(s) {{ return String(s).replace(/[&<>"]/g, c => ({{"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}})[c]); }}
// The reduced column: the high quality picture box filtered down to the icon's
// size with premultiplied alpha, as the renderer does, so the grey behind the
// model does not bleed into its edge
function reduce(img) {{
  const canvas = img.closest("tr").querySelector("canvas");
  if (!canvas || canvas.dataset.done) return;
  const w = img.naturalWidth, h = img.naturalHeight, f = Math.floor(w / ICON);
  if (!f) return;
  const src = document.createElement("canvas");
  src.width = w; src.height = h;
  const sctx = src.getContext("2d");
  sctx.drawImage(img, 0, 0);
  const p = sctx.getImageData(0, 0, w, h).data;
  const ctx = canvas.getContext("2d");
  const out = ctx.createImageData(ICON, ICON);
  for (let y = 0; y < ICON; y++) for (let x = 0; x < ICON; x++) {{
    let r = 0, g = 0, b = 0, a = 0;
    for (let dy = 0; dy < f; dy++) for (let dx = 0; dx < f; dx++) {{
      const i = ((y * f + dy) * w + (x * f + dx)) * 4, al = p[i + 3];
      r += p[i] * al; g += p[i + 1] * al; b += p[i + 2] * al; a += al;
    }}
    const o = (y * ICON + x) * 4;
    if (a) {{ out.data[o] = r / a; out.data[o + 1] = g / a; out.data[o + 2] = b / a; out.data[o + 3] = a / (f * f); }}
  }}
  ctx.putImageData(out, 0, 0);
  canvas.dataset.done = "1";
}}
function pic(src, cls, onload) {{
  return src ? `<td class="pic"><img loading="lazy" class="${{cls}}" src="${{src}}" alt=""${{onload ? ' onload="reduce(this)"' : ""}}></td>` : '<td class="pic">-</td>';
}}
function render() {{
  const q = document.getElementById("q").value.toLowerCase();
  const key = document.getElementById("sort").value;
  let list = rows.filter(r => !q || (r.name + " " + r.folder).toLowerCase().includes(q));
  const num = v => v === null ? -1 : v;
  if (key === "name") list.sort((a, b) => a.name.localeCompare(b.name));
  else if (key === "x1") list.sort((a, b) => num(a.x1) - num(b.x1));
  else if (key === "hq") list.sort((a, b) => num(a.hq) - num(b.hq));
  else list.sort((a, b) => a.folder.localeCompare(b.folder));
  tbody.innerHTML = list.map(r => `<tr>
    <td><div class="name">${{esc(r.name || r.short)}}</div><div class="id">${{esc(r.short)}}</div><div class="id">${{esc(r.stats)}}</div>${{r.note ? `<div class="warn">${{esc(r.note)}}</div>` : ""}}</td>
    ${{pic(r.orig, "", false)}}${{pic(r.hqimg, "smooth", true)}}<td class="pic">${{r.hqimg ? `<canvas width="${{ICON}}" height="${{ICON}}"></canvas>` : "-"}}</td>
    <td class="score"><div>exact: ${{esc(r.x1txt)}}</div><div>reduced: ${{esc(r.hqtxt)}}</div></td>
  </tr>`).join("");
  // a picture already decoded fires no load event for the new row
  for (const img of tbody.querySelectorAll("img[onload]")) if (img.complete && img.naturalWidth) reduce(img);
}}
for (const id of ["q", "sort"]) document.getElementById(id).addEventListener("input", render);
document.getElementById("checker").addEventListener("change", e => document.body.classList.toggle("checker", e.target.checked));
render();
</script>
</body>
</html>
"""


def write_standalone(rows, units, target, quality, max_mb):
    light = units[0].get("light", "")
    scale = units[0].get("scale", "") or "4"
    base, ext = os.path.splitext(target)
    ext = ext or ".html"
    parts = split_parts(rows, int(max_mb * 1000 * 1000))
    names = [base + ext] if len(parts) == 1 else [f"{base}-{i + 1}{ext}" for i in range(len(parts))]
    stem = os.path.basename(base)
    for i, (part, name) in enumerate(zip(parts, names)):
        if len(parts) > 1:
            links = []
            for j, (other, other_name) in enumerate(zip(parts, names)):
                first, last = group_of(other[0]["folder"]), group_of(other[-1]["folder"])
                label = f"{j + 1}: {first}" + (f" to {last}" if last != first else "")
                if i == j:
                    links.append(f'<span class="here">{html.escape(label)}</span>')
                else:
                    links.append(f'<a href="{html.escape(os.path.basename(other_name))}">{html.escape(label)}</a>')
            parts_html = '<div class="parts">Parts: ' + " ".join(links) + "</div>"
            heading = f"Unit icon re-render: {stem}, part {i + 1} of {len(parts)}"
            of_total = f" of {sum(len(p) for p in parts)}"
        else:
            parts_html = ""
            heading = f"Unit icon re-render: {stem}"
            of_total = ""
        keep = ("folder", "short", "stats", "name", "note", "x1", "hq", "x1txt", "hqtxt", "orig", "hqimg")
        page = STANDALONE_PAGE.format(
            title=html.escape(f"Icons {stem}" + (f" {i + 1}" if len(parts) > 1 else "")),
            heading=html.escape(heading),
            view=ICON * int(scale),
            icon=ICON,
            count=len(part),
            of_total=of_total,
            light=html.escape(light),
            scale=html.escape(scale),
            parts=parts_html,
            rows=json.dumps([{k: r[k] for k in keep} for r in part], ensure_ascii=False).replace("</", "<\\/"),
        )
        with open(name, "w", encoding="utf-8", newline="\n") as f:
            f.write(page)
        print(f"wrote {name}: {len(part)} units, {os.path.getsize(name) / 1e6:.1f} MB")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("out", help="the IconRender output directory (holds manifest.tsv)")
    ap.add_argument("--summary", action="store_true", help="print the scores only, write no page")
    ap.add_argument("--standalone", metavar="FILE.html", help="write one self-contained page (or numbered parts)")
    ap.add_argument("--quality", type=int, default=90, help="WebP quality of the high quality pictures (standalone)")
    ap.add_argument("--max-mb", type=float, default=9.5, help="split a standalone page larger than this")
    args = ap.parse_args()

    manifest = os.path.join(args.out, "manifest.tsv")
    with open(manifest, encoding="utf-8", newline="") as f:
        units = list(csv.DictReader(f, delimiter="\t"))
    if not units:
        sys.exit("empty manifest")

    report = os.path.join(args.out, "report")
    img_dir = os.path.join(report, "img")
    if not args.summary and not args.standalone:
        os.makedirs(img_dir, exist_ok=True)

    rows = []
    for n, u in enumerate(units):
        unit_dir = os.path.join(args.out, u["folder"])
        orig = load_rgba(os.path.join(unit_dir, "orig.dds"))
        x1 = load_rgba(os.path.join(unit_dir, "x1.tga"))
        hq = load_rgba(os.path.join(unit_dir, "hq.tga"))
        small = reduce(hq, ICON) if hq is not None else None
        # Only a 42x42 original can be scored; a few shipped icons are other sizes.
        ref = orig if orig is not None and orig.shape[:2] == (ICON, ICON) else None
        s_x1 = scores(ref, x1)
        s_hq = scores(ref, small)

        notes = []
        if u["status"] != "ok":
            notes.append(u["status"])
        corrections = u.get("corrections", "")
        if corrections:
            notes.append("corrected: " + corrections)
        elif u["camera"] != "stats":
            notes.append("no saved camera, framing guessed")
        if orig is None:
            notes.append("no shipped icon")
        elif ref is None:
            notes.append(f"shipped icon is {orig.shape[1]}x{orig.shape[0]}")

        row = {
            "folder": u["folder"],
            # the unit's folder name, and the record's own name when the folder holds a second one
            "short": "/".join(u["stats"].split("/")[-2:]) if not u["stats"].lower().endswith("/mechunitrpgstats.xdb")
            else u["folder"].rstrip("/").split("/")[-1],
            "stats": u["stats"],
            "name": u["name"],
            "note": ", ".join(notes),
            "x1": None if s_x1 is None else s_x1["ssim"],
            "hq": None if s_hq is None else s_hq["ssim"],
            "x1txt": fmt_score(s_x1),
            "hqtxt": fmt_score(s_hq),
            "orig": None,
            "hqimg": None,
            "small": None,
            "x1img": None,
            "psnr_x1": None if s_x1 is None else s_x1["psnr"],
            "psnr_hq": None if s_hq is None else s_hq["psnr"],
        }
        if args.standalone:
            row["orig"] = webp_b64(orig, True, 0) if orig is not None else None
            row["hqimg"] = webp_b64(hq, False, args.quality) if hq is not None else None
            row["bytes"] = len(row["orig"] or "") + len(row["hqimg"] or "") + 600
        elif not args.summary:
            for key, img in (("orig", orig), ("hqimg", hq), ("small", small), ("x1img", x1)):
                if img is not None:
                    name = f"{n:04d}_{key}.png"
                    save_png(img, os.path.join(img_dir, name))
                    row[key] = "img/" + name
        rows.append(row)

    def stat(key):
        v = [r[key] for r in rows if r[key] is not None and r[key] != float("inf")]
        return (np.median(v), np.min(v), len(v)) if v else (float("nan"), float("nan"), 0)

    for label, key in (("exact SSIM", "x1"), ("exact PSNR", "psnr_x1"), ("reduced SSIM", "hq"), ("reduced PSNR", "psnr_hq")):
        med, low, cnt = stat(key)
        print(f"{label:13s} median {med:8.3f}  worst {low:8.3f}  over {cnt} units")

    if args.summary:
        return
    if args.standalone:
        write_standalone(rows, units, args.standalone, args.quality, args.max_mb)
        return
    light = units[0].get("light", "")
    scale = units[0].get("scale", "")
    page = PAGE.format(
        view=ICON * int(scale or 4),
        icon=ICON,
        count=len(rows),
        light=html.escape(light),
        scale=html.escape(scale),
        rows=json.dumps(rows).replace("</", "<\\/"),
    )
    with open(os.path.join(report, "index.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(page)
    print("wrote", os.path.join(report, "index.html"))


if __name__ == "__main__":
    main()
