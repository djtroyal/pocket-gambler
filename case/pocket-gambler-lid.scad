// Pocket Gambler — Rear Lid
// Snaps onto the body's lid_lip_ledge, or secure with 4× M2×6 screws

// ═══════════════════════════════════════════════════
// PARAMETERS (must match body.scad)
// ═══════════════════════════════════════════════════

body_w     = 75;
body_h     = 110;
corner_r   = 4;
lid_t      = 2.2;    // lid plate thickness
lid_lip_t  = 1.2;    // lip thickness (must match body)
lid_lip_d  = 1.8;    // lip depth (must match body)

// Battery door opening
batt_door_w = 37;
batt_door_h = 58;
batt_door_x = (body_w - batt_door_w) / 2;
batt_door_y = (body_h - batt_door_h) / 2 - 5;
door_corner = 2;

// Screw holes (optional — use instead of or in addition to snap clips)
use_screw_holes = true;
screw_dia  = 2.5;    // M2 clearance hole
screw_margin = 4;    // distance from corner

// Snap clip parameters (flexible tabs)
use_snap_clips = false;  // set true if not using screws
clip_w  = 8;
clip_h  = 4;
clip_t  = 1.2;
clip_protrusion = 0.8;


// ═══════════════════════════════════════════════════
// MODULES
// ═══════════════════════════════════════════════════

module rounded_plate(w, h, t, r) {
    minkowski() {
        cube([w - 2*r, h - 2*r, t - r]);
        translate([r, r, r]) sphere(r=r, $fn=20);
    }
}

module lid_lip() {
    // Perimeter lip that inserts into the body opening
    difference() {
        cube([body_w, body_h, lid_lip_d]);
        translate([lid_lip_t, lid_lip_t, -0.1])
            cube([body_w - 2*lid_lip_t, body_h - 2*lid_lip_t, lid_lip_d + 0.2]);
    }
}

module battery_door_opening() {
    translate([batt_door_x, batt_door_y, -0.1])
        minkowski() {
            cube([batt_door_w - 2*door_corner,
                  batt_door_h - 2*door_corner,
                  lid_t + 0.2]);
            cylinder(r=door_corner, h=0.01, $fn=12);
        }
}

module screw_holes() {
    if (use_screw_holes) {
        positions = [
            [screw_margin, screw_margin],
            [body_w - screw_margin, screw_margin],
            [screw_margin, body_h - screw_margin],
            [body_w - screw_margin, body_h - screw_margin]
        ];
        for (p = positions) {
            translate([p[0], p[1], -0.1])
                cylinder(d=screw_dia, h=lid_t + 0.2, $fn=16);
        }
    }
}

module snap_clips() {
    if (use_snap_clips) {
        // 4 flexible clips on inside perimeter
        // Left edge clip
        translate([0, body_h/2 - clip_w/2, lid_t])
            cube([clip_t, clip_w, clip_h]);
        // Right edge clip
        translate([body_w - clip_t, body_h/2 - clip_w/2, lid_t])
            cube([clip_t, clip_w, clip_h]);
        // Top edge clip
        translate([body_w/2 - clip_w/2, body_h - clip_t, lid_t])
            cube([clip_w, clip_t, clip_h]);
        // Bottom edge clip
        translate([body_w/2 - clip_w/2, 0, lid_t])
            cube([clip_w, clip_t, clip_h]);
    }
}


// ═══════════════════════════════════════════════════
// MAIN ASSEMBLY
// ═══════════════════════════════════════════════════

difference() {
    union() {
        rounded_plate(body_w, body_h, lid_t, corner_r);
        translate([0, 0, lid_t - 0.01]) lid_lip();
    }
    battery_door_opening();
    screw_holes();
}

snap_clips();
