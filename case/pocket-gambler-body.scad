// Pocket Gambler — Main Body
// OpenSCAD parametric design
// Render: F6 | Export STL: File > Export > Export as STL

// ═══════════════════════════════════════════════════
// PARAMETERS — adjust to fit your printer tolerances
// ═══════════════════════════════════════════════════

// Outer case dimensions
body_w   = 75;    // width  (X)
body_h   = 110;   // height (Y)
body_d   = 18;    // depth  (Z, front face to back)
wall_t   = 2.2;   // wall thickness
corner_r = 4;     // corner radius

// Interior cavity (for PCB + components)
inner_w = body_w - 2 * wall_t;
inner_h = body_h - 2 * wall_t;
inner_d = body_d - wall_t;        // open at back for lid

// Display window (SSD1306 0.96" active area ~26mm x 14mm)
disp_w      = 28;    // cutout width (2mm margin around active area)
disp_h      = 16;    // cutout height
disp_x      = (body_w - disp_w) / 2;
disp_y      = body_h - 38;        // upper section of front face
disp_recess = 1.2;   // depth of diffuser recess (front face inset)
disp_lip    = 1.5;   // extra margin for recess (holds diffuser)

// Button holes
btn_dia   = 6.5;     // 6mm button + 0.5mm tolerance
btn_recess_d   = 1.5;   // depth of recess pocket on outer face
btn_recess_dia = 9.0;   // diameter of recess pocket

// D-pad center position
dpad_cx = 20;
dpad_cy = 52;
dpad_s  = 11;    // center-to-center spacing

// Action buttons (A, B)
ab_cx   = body_w - 20;
ab_cy   = 52;
ab_offset = 10; // half-diagonal offset

// START / SELECT positions
start_x  = body_w * 0.38;
select_x = body_w * 0.62;
ss_y     = 34;

// USB cutout (side, for Pico micro-USB)
usb_w = 10;
usb_h =  5;
usb_y = 5;     // from bottom edge

// Power switch slot (opposite side from USB)
sw_slot_w = 13;
sw_slot_h =  5;
sw_slot_y = 8;

// Battery compartment opening in rear (for lid access)
batt_door_w  = 37;
batt_door_h  = 58;
batt_door_x  = (body_w - batt_door_w) / 2;
batt_door_y  = (body_h - batt_door_h) / 2 - 5;

// PCB standoffs (M2 screws, 5mm tall)
pcb_w  = 65;   // PCB width
pcb_h  = 90;   // PCB height
pcb_x  = (body_w - pcb_w) / 2;
pcb_y  = (body_h - pcb_h) / 2;
boss_od = 4.5;  // outer diameter
boss_id = 2.3;  // inner hole diameter (M2 tap or self-tap)
boss_h  = 5.5;  // height of standoff from inner floor

// Lid retention: lip on inside edge of body opening
lid_lip_d = 1.8;   // how far lid lip inserts
lid_lip_t = 1.2;   // thickness of lid lip ledge

// Speaker grille (for buzzer)
grille_cx     = body_w * 0.5;
grille_cy_bot = 10;   // from bottom
grille_rows   = 2;
grille_cols   = 7;
grille_hole_d = 1.8;
grille_pitch  = 3.2;


// ═══════════════════════════════════════════════════
// MODULES
// ═══════════════════════════════════════════════════

module rounded_box(w, h, d, r) {
    // Minkowski sum of box and small sphere for rounded edges
    minkowski() {
        cube([w - 2*r, h - 2*r, d - r]);
        translate([r, r, r]) sphere(r=r, $fn=24);
    }
}

module display_window() {
    // Full through-hole
    translate([disp_x, disp_y, -0.1])
        cube([disp_w, disp_h, wall_t + 0.2]);
    // Diffuser recess (wider/taller, shallower)
    translate([disp_x - disp_lip, disp_y - disp_lip, wall_t - disp_recess])
        cube([disp_w + 2*disp_lip, disp_h + 2*disp_lip, disp_recess + 0.1]);
}

module button_hole(x, y) {
    // Through hole
    translate([x, y, -0.1])
        cylinder(d=btn_dia, h=wall_t + 0.2, $fn=20);
    // Recess pocket on outer face
    translate([x, y, wall_t - btn_recess_d])
        cylinder(d=btn_recess_dia, h=btn_recess_d + 0.1, $fn=20);
}

module dpad_buttons() {
    button_hole(dpad_cx,          dpad_cy + dpad_s);   // UP
    button_hole(dpad_cx,          dpad_cy - dpad_s);   // DOWN
    button_hole(dpad_cx - dpad_s, dpad_cy);            // LEFT
    button_hole(dpad_cx + dpad_s, dpad_cy);            // RIGHT
}

module ab_buttons() {
    button_hole(ab_cx + ab_offset, ab_cy);             // A (right)
    button_hole(ab_cx,             ab_cy + ab_offset); // B (upper)
}

module start_select_buttons() {
    button_hole(start_x,  ss_y);
    button_hole(select_x, ss_y);
}

module usb_cutout() {
    // Right side wall cutout
    translate([body_w - wall_t - 0.1, (body_h - usb_w) / 2, usb_y])
        cube([wall_t + 0.2, usb_w, usb_h]);
}

module power_switch_slot() {
    // Left side wall slot
    translate([-0.1, (body_h - sw_slot_w) / 2, sw_slot_y])
        cube([wall_t + 0.2, sw_slot_w, sw_slot_h]);
}

module boss(x, y) {
    translate([x, y, wall_t])
        difference() {
            cylinder(d=boss_od, h=boss_h, $fn=16);
            translate([0, 0, -0.1])
                cylinder(d=boss_id, h=boss_h + 0.2, $fn=16);
        }
}

module pcb_standoffs() {
    // 4 corners of PCB footprint
    boss(pcb_x,          pcb_y);
    boss(pcb_x + pcb_w,  pcb_y);
    boss(pcb_x,          pcb_y + pcb_h);
    boss(pcb_x + pcb_w,  pcb_y + pcb_h);
}

module grille() {
    gx0 = grille_cx - (grille_cols - 1) * grille_pitch / 2;
    gy0 = grille_cy_bot;
    for (r = [0:grille_rows-1]) {
        for (c = [0:grille_cols-1]) {
            translate([gx0 + c * grille_pitch, gy0 + r * grille_pitch, -0.1])
                cylinder(d=grille_hole_d, h=wall_t + 0.2, $fn=8);
        }
    }
}

module lid_lip_ledge() {
    // Inner ledge around the back opening that the lid clips onto
    translate([0, 0, wall_t])
        difference() {
            cube([body_w, body_h, lid_lip_d]);
            translate([lid_lip_t, lid_lip_t, -0.1])
                cube([body_w - 2*lid_lip_t, body_h - 2*lid_lip_t, lid_lip_d + 0.2]);
        }
}

// ═══════════════════════════════════════════════════
// MAIN ASSEMBLY
// ═══════════════════════════════════════════════════

difference() {
    // Outer shell
    rounded_box(body_w, body_h, body_d, corner_r);

    // Hollow interior (open at back, Z+)
    translate([wall_t, wall_t, wall_t])
        cube([inner_w, inner_h, body_d]);

    // Front face features
    display_window();
    dpad_buttons();
    ab_buttons();
    start_select_buttons();
    grille();

    // Side cutouts
    usb_cutout();
    power_switch_slot();
}

// Internal additions (added after subtract)
pcb_standoffs();
lid_lip_ledge();
