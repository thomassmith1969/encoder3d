# Contributing to Encoder3D CNC Controller

First off, thank you for considering contributing to Encoder3D! It's people like you that make this project better for everyone.

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inspiring community for all. Please be respectful and constructive in all interactions.

### Expected Behavior

- Use welcoming and inclusive language
- Be respectful of differing viewpoints and experiences
- Gracefully accept constructive criticism
- Focus on what is best for the community
- Show empathy towards other community members

### Unacceptable Behavior

- Trolling, insulting/derogatory comments, and personal attacks
- Public or private harassment
- Publishing others' private information without permission
- Other conduct which could reasonably be considered inappropriate

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates.

**When submitting a bug report, include:**

- **Clear title** - Brief, descriptive summary
- **Description** - Detailed explanation of the issue
- **Steps to reproduce** - Exact steps to trigger the bug
- **Expected behavior** - What should happen
- **Actual behavior** - What actually happens
- **Hardware setup** - Board, motors, drivers, etc.
- **Software version** - Firmware commit/version
- **Logs** - Serial output, error messages
- **Photos/videos** - If relevant

**Template:**
```markdown
## Bug Description
[Clear description of the bug]

## Hardware
- Board: Lolin32 Lite
- Motors: [Your motor specifications]
- Drivers: [Your driver model]

## Steps to Reproduce
1. [First step]
2. [Second step]
3. [See error]

## Expected Behavior
[What should happen]

## Actual Behavior
[What actually happens]

## Logs
```
[Paste serial output or logs]
```

## Additional Context
[Any other relevant information]
```

### Suggesting Features

Feature suggestions are welcome! Please provide:

- **Use case** - Why is this feature needed?
- **Proposed solution** - How should it work?
- **Alternatives considered** - Other approaches
- **Impact** - Who benefits from this feature?

### Pull Requests

We actively welcome your pull requests!

**Process:**

1. **Fork** the repository
2. **Create a branch** from `main`
   ```bash
   git checkout -b feature/my-feature
   ```
3. **Make your changes**
   - Follow code style guidelines
   - Add comments where needed
   - Update documentation
4. **Test thoroughly**
   - Test on actual hardware if possible
   - Verify all existing features still work
5. **Commit** with clear messages
   ```bash
   git commit -m "Add feature: description of feature"
   ```
6. **Push** to your fork
   ```bash
   git push origin feature/my-feature
   ```
7. **Open a Pull Request**
   - Describe what changed and why
   - Reference any related issues
   - Include test results

**Pull Request Template:**
```markdown
## Description
[Brief description of changes]

## Related Issues
Fixes #[issue number]

## Changes Made
- [Change 1]
- [Change 2]

## Testing
- [ ] Tested on hardware
- [ ] Verified existing features work
- [ ] Updated documentation
- [ ] Added/updated comments

## Hardware Tested On
- Board: [Your board]
- Configuration: [Your setup]

## Additional Notes
[Any other relevant information]
```

## Development Guidelines

### Code Style

**C++ Guidelines:**
- Use consistent indentation (4 spaces)
- Use descriptive variable and function names
- Add comments for complex logic
- Follow existing naming conventions:
  - Classes: `PascalCase`
  - Functions/methods: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
  - Variables: `snake_case`

**Example:**
```cpp
class MotorController {
private:
    float current_position;
    const float MAX_SPEED = 100.0;
    
public:
    void setTargetPosition(float target);
    float getCurrentPosition();
};
```

**JavaScript Guidelines:**
- Use ES6+ features
- Use `const` and `let`, avoid `var`
- Use descriptive names
- Add JSDoc comments for functions

### Documentation

**Code Comments:**
- Explain **why**, not just **what**
- Document parameters and return values
- Note any hardware dependencies
- Warn about safety considerations

**Example:**
```cpp
/**
 * Sets the target temperature for a heater with safety checks
 * 
 * @param heater_id Index of heater (HEATER_HOTEND or HEATER_BED)
 * @param temp Target temperature in Celsius
 * 
 * WARNING: Always verify thermistor is connected before heating
 */
void setTemperature(uint8_t heater_id, float temp);
```

**README Updates:**
- Update README.md if adding features
- Add examples for new functionality
- Update API documentation if changed

### Testing

**Before submitting:**

1. **Compile** - Ensure code compiles without errors
   ```bash
   pio run
   ```

2. **Upload** - Test on actual hardware when possible
   ```bash
   pio run --target upload
   ```

3. **Verify** - Check that:
   - New feature works as intended
   - Existing features still work
   - No new warnings or errors
   - Safety features remain functional

4. **Document** - Note any special testing procedures

### Safety Considerations

This firmware controls potentially dangerous hardware. When contributing:

- **Never bypass safety features**
- **Test heater code extremely carefully**
- **Verify emergency stop always works**
- **Document any safety implications**
- **Consider thermal runaway scenarios**
- **Validate all inputs and limits**

## Project Structure

```
encoder3d/
├── include/          # Header files
├── src/              # Implementation files
├── data/www/         # Web interface files
├── docs/             # Additional documentation
├── platformio.ini    # Build configuration
└── README.md         # Main documentation
```

### Adding New Files

- Place headers in `include/`
- Place implementations in `src/`
- Update `platformio.ini` if adding libraries
- Document in README.md

## Commit Messages

Use clear, descriptive commit messages:

**Good:**
```
Add PID auto-tuning for heaters
Fix encoder overflow on long moves
Update web UI with real-time graphs
```

**Not ideal:**
```
fixed stuff
Update
changes
```

**Format:**
```
<type>: <brief description>

[Optional detailed description]

[Optional references to issues]
```

**Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation changes
- `style:` Code style/formatting
- `refactor:` Code refactoring
- `test:` Adding tests
- `chore:` Maintenance tasks

## License

By contributing, you agree that your contributions will be licensed under the same MIT License that covers this project. See [LICENSE](LICENSE) file for details.

## Questions?

Don't hesitate to ask questions! Use:
- GitHub Issues for bug reports
- GitHub Discussions for questions
- Pull Request comments for code-specific questions

## Recognition

Contributors will be recognized in:
- GitHub contributors page
- Release notes
- Project credits

Thank you for helping make Encoder3D better! 🚀

---

**Remember**: This is a community project. We're all learning and building together. Don't be afraid to contribute, no matter how small the change!
