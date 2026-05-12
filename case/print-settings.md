# 3D Print Settings — Pocket Gambler Case

## Parts

| File | Description | Print time (est.) |
|------|-------------|-------------------|
| `pocket-gambler-body.scad` | Main case shell | ~3–4 hrs |
| `pocket-gambler-lid.scad` | Rear panel | ~1–2 hrs |
| `pocket-gambler-battery-door.scad` | Battery compartment door | ~30 min |

## Rendering STL Files

Open each `.scad` file in [OpenSCAD](https://openscad.org) (free), press **F6** to render,
then **File → Export → Export as STL**.

Or from the command line:
```bash
openscad -o pocket-gambler-body.stl pocket-gambler-body.scad
openscad -o pocket-gambler-lid.stl pocket-gambler-lid.scad
openscad -o pocket-gambler-battery-door.stl pocket-gambler-battery-door.scad
```

---

## Recommended Settings

### Material
- **PETG** (recommended) — slightly flexible, good layer adhesion, snaps/clips work well
- **PLA** — acceptable; snap clips may be brittle. Increase clip_t to 1.5mm if using PLA

### Slicer Settings (tested with PrusaSlicer / Bambu Studio / Cura)

| Setting | Value |
|---------|-------|
| Layer height | 0.15mm |
| First layer height | 0.2mm |
| Perimeters/walls | 3 |
| Top/bottom layers | 5 |
| Infill | 25% Gyroid |
| Infill extrusion width | 0.45mm |
| Support | Tree supports, 10° threshold |
| Support interface | 0.2mm |
| Brim | 5mm (body only, optional) |

### Print Orientation

- **Body:** Display window face-down on build plate. This gives the best surface finish
  on the display window recess and button faces.
  > Supports required under the display window recess.

- **Lid:** Exterior face-down. Flat surface, no supports needed.

- **Battery door:** Exterior face-down. No supports needed.

---

## Tolerance Adjustments

If buttons don't move freely, increase `btn_dia` in `body.scad`:
```
btn_dia = 6.5;   // default; try 6.7 or 7.0 for loose-toleranced printers
```

If the lid doesn't fit, adjust `lid_lip_t` (thinner = looser fit):
```
lid_lip_t = 1.2;   // try 1.0 for a looser fit
```

If battery door slides too tightly, increase clearance:
```
door_w = 36.4;   // increase by 0.2–0.4mm for looser fit
door_h = 57.4;   // same
```

---

## Post-Processing

1. **Display window:** Lightly sand the diffuser recess with 400-grit sandpaper.
   Cut a 30mm × 18mm piece of frosted acrylic or translucent white film to fit
   in the recess ledge. Secure with a small drop of CA glue if needed.

2. **Button holes:** Test all 8 buttons fit through the holes before final assembly.
   A 6mm drill bit can be used to chase any holes that print tight.

3. **Thread tapping:** The PCB standoffs (4.5mm OD, 2.3mm hole) can be tapped M2
   or used directly with M2 self-tapping screws (M2×6 pan head).

4. **Snap finish:** Light sanding with 220-grit on all mating surfaces will improve
   the lid fit and give a cleaner appearance.

---

## Assembly Order

See `docs/assembly-guide.md` for the complete step-by-step assembly procedure.

---

## BOM — Printed Parts

| Part | Material | Weight (est.) |
|------|----------|---------------|
| Body | PETG | ~40g |
| Lid | PETG | ~12g |
| Battery door | PETG | ~5g |

Total filament: ~57g (about 19m of 1.75mm PETG)
