# Copilot Instructions for TTGO T-HIGrow Project

- Start every message and response with my name.
- Always talk to me in German.

## Code Style Guidelines

### Language Requirements
- **All code must be written in English**
- **All code comments must be written in English**
- **All variable names, function names, and identifiers must be in English**
- **All log output messages must be in English**
- **All documentation strings and inline comments must be in English**

### Formatting and Style
- **No emojis are allowed anywhere in the codebase**
  - No emojis in code comments
  - No emojis in log output messages
  - No emojis in variable names or identifiers
  - No emojis in documentation
- Use clear, descriptive English for all naming conventions
- Follow consistent C++ coding standards
- Use meaningful variable and function names

### Documentation Standards
- Write clear, concise comments explaining complex logic
- Document function parameters and return values
- Use proper English grammar in all documentation
- Avoid informal language or slang

### Examples

#### ✅ Correct:
```cpp
// Initialize the temperature sensor
float temperature = readTemperatureSensor();
Serial.println("Temperature reading completed successfully");
```

#### ❌ Incorrect:
```cpp
// Temperatur-Sensor initialisieren 🌡️
float temp = readTempSensor();
Serial.println("Temp reading done! 🎉");
```

### Platform-Specific Guidelines
- For Arduino/ESP32 projects: Follow Arduino IDE naming conventions
- For PlatformIO projects: Use consistent library naming
- Maintain compatibility with existing codebase standards

### Exception Handling
- All error messages must be in English
- Use descriptive error messages without emojis
- Log levels should use standard English terminology

### Version Control
- Commit messages should be in English
- Branch names should use English descriptors
- Pull request descriptions must be in English
- Never name Claude as committer or co-author on commits (no "Co-Authored-By: Claude" trailer, no Claude author identity)

## Enforcement
These guidelines apply to:
- All new code contributions
- Code refactoring and updates
- Documentation updates
- Configuration files
- README and changelog updates

## Legacy Code
When updating existing code that doesn't follow these guidelines:
- Gradually migrate to English where possible
- Prioritize consistency within modified sections
- Update comments to English when touching related code
