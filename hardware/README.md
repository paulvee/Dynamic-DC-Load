# Hardware Documentation

This directory contains all hardware design files, schematics, PCB layouts, and Bill of Materials for the Dynamic DC Load project.

---

## 📁 Directory Structure

```
hardware/
├── schematics/          # Circuit schematics (PDF, PNG)
├── pcb/                 # PCB design files (KiCAD)
├── bom/                 # Bill of Materials
├── enclosure/           # 3D-printed enclosure designs
└── README.md           # This file
```

---

## 📋 Contents

### Schematics
- **ElectronicLoad_DC_10A_V51a.pdf** - Complete schematic diagram (version 5.1a)
- **Schematic V5.1a.png** - Schematic image for quick reference

### PCB Files (KiCAD)
- **ElectronicLoad_DC_10A_V51a.kicad_pcb.zip** - Main board PCB design
- **BackPlateV1.kicad_pcb.zip** - Back plate PCB
- **FacePlateV1.1.kicad_pcb.zip** - Front panel PCB (version 1.1)

### Bill of Materials
- **BOM_V1.b.xlsx** - Complete component list with part numbers
- **Special Parts BOM_V1a.pdf** - Special/hard-to-find components

### Enclosure
- **3d-Print enclosure.zip** - 3D-printable enclosure designs (STL files)

---

## 🔧 Hardware Version

**Current Version:** 5.1a  
**PCB Version:** V1.1 (front plate), V1.0 (others)  
**Last Updated:** 2026

---

## 📐 Specifications

- **Load Capacity:** Up to 10A continuous
- **Voltage Range:** 0-60V
- **Power Dissipation:** Depends on heatsink
- **Control:** ESP32-based with OLED display
- **Interface:** USB serial, rotary encoder

---

## 🛠️ Building the Hardware

### Required Tools
- Soldering iron and solder
- Multimeter for testing
- Heat-shrink tubing
- Wire cutters and strippers
- 3D printer (for enclosure)

### Assembly Order
1. Review schematic carefully
2. Order components from BOM
3. Fabricate PCBs (or order from PCB manufacturer)
4. Solder components starting with smallest parts
5. Test each section before final assembly
6. Flash firmware (see `../firmware/`)
7. Calibrate using firmware calibration mode
8. Print and assemble enclosure

### PCB Manufacturing Notes
- **Layers:** 2-layer PCB
- **Thickness:** 1.6mm standard
- **Copper:** 2oz recommended (high current paths)
- **Finish:** HASL or ENIG
- **Minimum hole size:** Check design files

---

## ⚠️ Safety Notes

**WARNING:** This device handles high currents and power. Improper assembly or use can result in:
- Fire hazard
- Component damage
- Personal injury

**Safety Requirements:**
- ✅ Use proper heatsinks with thermal compound
- ✅ Ensure adequate ventilation (fan included in design)
- ✅ Use appropriate wire gauge for current capacity
- ✅ Double-check polarity before connecting power
- ✅ Test at low power before full-scale operation
- ✅ Never exceed component ratings
- ✅ Use in well-ventilated area
- ✅ Keep combustible materials away

---

## 📦 Special Components

Some components may require specific suppliers:
- **Power MOSFETs** - High current rating, TO-220 package
- **Shunt resistor** - Low resistance, high precision, high power
- **DAC8571** - 16-bit DAC (Texas Instruments)
- **ADS1015** - 12-bit ADC (Texas Instruments)
- **SSD1351** - 128x128 OLED display
- **Heat sink** - Adequate thermal capacity for expected power dissipation

See `bom/Special Parts BOM_V1a.pdf` for detailed specifications.

---

## 🔗 Related Documentation

- **Firmware:** See `../firmware/` directory
- **Photos:** See `../docs/images/` for build photos
- **User Guide:** See main repository README

---

## 📝 Design Notes

- PCB designed using KiCAD (open-source EDA tool)
- Optimized for DIY assembly with through-hole and large SMD components
- Modular design with separate control and power sections
- Fan control integrated for thermal management
- Calibration system in firmware eliminates need for precision resistors

---

## 📞 Support

For hardware questions or issues:
- **GitHub Issues:** [https://github.com/paulvee/Dynamic-DC-Load/issues](https://github.com/paulvee/Dynamic-DC-Load/issues)
- Label hardware-related issues with `hardware` tag

---

**Copyright © 2026 Paul Versteeg**
