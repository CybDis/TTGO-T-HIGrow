# LilyGO TTGO T-HiGrow
## TTGO T-HIGrow MQTT autodiscover interface for Homeassistant

![T-Higrow](images/T-Higrow.jpg)

> **NEW in v5.0.0**: Native Home Assistant Auto Discovery integration! No more external Python scripts needed.

See [CHANGELOG.md](CHANGELOG.md) for detailed release notes.

### Getting started

Refer to the [Setup & Configuration Guide](howto.md) for comprehensive instructions on installation, configuration, and troubleshooting.

## Disclaimer

The project is forked from [@pesor](https://github.com/pesor) - who did a great work. The original repo is deleted meanwhile.

**Primary modifications are:**

- Updates in MQTT messages to reflect latest Home Assistant changes
- Auto Discovery for Home Assistant
- New Casing (3D print stl files) supporting larger LiPo battery
- Original Casing (3D print stl files)
- Some Bugfixing and optimizations

## Home Assistant Integration

![Home Assistant Custom Plant](images/HomeAssistant_CustomPlant.png)

These sensors are integrated into a comprehensive smart home plant monitoring system using Home Assistant. The setup utilizes:

- **[Flower Card (Mod)](https://github.com/CybDis/lovelace-flower-card-mod)** - A custom Lovelace card for displaying plant status and health metrics
- **[Custom Plant Integration](https://github.com/Olen/homeassistant-plant)** - Home Assistant integration that combines sensor data to provide plant health monitoring and care alerts

This creates a complete plant care management system with real-time monitoring, alerts, and visual dashboards directly in Home Assistant.

## 3D Cases

![HiGrow Cases](images/HiGrow_Cases_New.png)

The new custom casing has been optimized to accommodate a **1100 mAh LiPo battery**, providing extended battery life for the plant monitoring sensor. The 3D printable STL files are available in the [Casing/Custom](Casing/Custom) directory for users who want to build their own housing with the larger battery capacity.

Original casing files from [@pesor](https://github.com/pesor) are available in the [Casing/Original](Casing/Original) directory for reference.

## Support my work ...
**...with caffeine?**

<a href="https://www.buymeacoffee.com/cybdis" target="_blank">
  <img src="https://raw.githubusercontent.com/CybDis/CybDis/main/bmc-yellow-button.png" height="60px"/></a>

