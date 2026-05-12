// Pocket Gambler — Battery Door
// Sliding door for 2× AAA battery compartment
// Fits into the battery_door opening in the rear lid

// ═══════════════════════════════════════════════════
// PARAMETERS (must match lid.scad opening)
// ═══════════════════════════════════════════════════

door_w    = 36.4;   // slightly smaller than opening (0.6mm clearance per side)
door_h    = 57.4;   // slightly smaller than opening
door_t    = 2.0;    // door plate thickness
corner_r  = 1.8;

// Lip around 3 edges (holds door in opening, open 4th side for sliding)
lip_t     = 1.0;    // lip thickness (overlaps lid edge)
lip_h     = 1.5;    // how much lip wraps around lid thickness

// Thumb notch (half-circle cutout for opening)
notch_r   = 6;
notch_x   = door_w / 2;
notch_y   = door_h;   // at the open (sliding) edge

// AAA cell guides (internal ribs to hold batteries in place)
// 2× AAA in parallel orientation (side by side)
aaa_d      = 10.5;   // AAA cell diameter + 0.5mm clearance
aaa_len    = 45.0;   // AAA cell length + 1mm clearance
aaa_wall   = 1.2;    // rib wall thickness
aaa_sep    = 1.5;    // gap between cells
aaa_y_off  = 6;      // from bottom of door interior

// Total cell bay width = 2 × aaa_d + aaa_sep + 2 × aaa_wall
// Must fit within door_w
bay_w  = 2 * aaa_d + aaa_sep + 2 * aaa_wall;
bay_x  = (door_w - bay_w) / 2;


// ═══════════════════════════════════════════════════
// MODULES
// ═══════════════════════════════════════════════════

module rounded_plate(w, h, t, r) {
    hull() {
        translate([r, r, 0])       cylinder(r=r, h=t, $fn=20);
        translate([w-r, r, 0])     cylinder(r=r, h=t, $fn=20);
        translate([r, h-r, 0])     cylinder(r=r, h=t, $fn=20);
        translate([w-r, h-r, 0])   cylinder(r=r, h=t, $fn=20);
    }
}

module thumb_notch() {
    translate([notch_x, notch_y, -0.1])
        cylinder(r=notch_r, h=door_t + 0.2, $fn=20);
}

module edge_lips() {
    // Left lip
    translate([-lip_t, 0, 0])
        cube([lip_t, door_h, door_t + lip_h]);
    // Right lip
    translate([door_w, 0, 0])
        cube([lip_t, door_h, door_t + lip_h]);
    // Bottom lip (closed end)
    translate([-lip_t, -lip_t, 0])
        cube([door_w + 2*lip_t, lip_t, door_t + lip_h]);
    // Note: top edge (notch end) has no lip — this is the sliding direction
}

module aaa_cell_guides() {
    // Two parallel cradles for AAA cells
    guide_h = 4;  // height of rib wall above door floor

    // Left outer wall
    translate([bay_x, aaa_y_off, door_t])
        cube([aaa_wall, aaa_len, guide_h]);

    // Center divider
    translate([bay_x + aaa_wall + aaa_d, aaa_y_off, door_t])
        cube([aaa_sep, aaa_len, guide_h]);

    // Right outer wall
    translate([bay_x + aaa_wall + aaa_d + aaa_sep + aaa_d, aaa_y_off, door_t])
        cube([aaa_wall, aaa_len, guide_h]);

    // End stops (short ribs at both ends of each cell bay)
    for (cell = [0:1]) {
        cx = bay_x + aaa_wall + cell * (aaa_d + aaa_sep);
        // Front end stop
        translate([cx, aaa_y_off, door_t])
            cube([aaa_d, aaa_wall, guide_h]);
        // Rear end stop
        translate([cx, aaa_y_off + aaa_len - aaa_wall, door_t])
            cube([aaa_d, aaa_wall, guide_h]);
    }
}


// ═══════════════════════════════════════════════════
// MAIN ASSEMBLY
// ═══════════════════════════════════════════════════

difference() {
    union() {
        rounded_plate(door_w, door_h, door_t, corner_r);
        edge_lips();
    }
    thumb_notch();
}

aaa_cell_guides();
