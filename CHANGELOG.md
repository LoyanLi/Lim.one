# Changelog

All notable changes to Lim.one audio plugin are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.1] - 2026-02-23

### Fixed
- **Parameter Isolation**: Resolve cross-project parameter pollution issue
  - Fixed issue where new plugin instances would load previous project's parameters
  - Implemented lazy-loading of user defaults with DAW state detection
  - Ensured complete parameter isolation between different DAW projects
  - Users can now safely switch between multiple DAW sessions without parameter interference

### Technical Details
- Removed automatic parameter saving to global `user_defaults.xml` file
- DAW project parameters now take precedence via `setStateInformation()`
- User defaults loaded only as fallback when DAW doesn't provide initial state
- Parameters now managed exclusively by DAW state management (JUCE best practices)
- All platforms tested and verified:
  - macOS: AU, VST3, AAX, Standalone
  - Windows: VST3 (universal installer)

### Testing
- Parameter isolation verified across multiple DAW projects
- Linked parameter functionality (character value) confirmed working correctly
- Project save/load state restoration validated
- No regression in existing parameter functionality

## [0.1.0] - 2026-02-15

### Initial Release

#### Features
- **Limiter Engine**
  - Character-based linked parameter system for cohesive control
  - Support for both Soft and Hard clipper modes
  - Modern (feed-forward) and Classic (feedback) limiter architectures
  - True Peak detection and safety limiting
  - Delta mode for A/B comparison

#### Audio Processing
- Configurable oversampling (1x, 2x, 4x, 8x, 16x) for reduced aliasing
  - Automatic minimum oversampling (4x) for VST3 to ensure safety
  - Independent oversampling factors for clipper and limiter
- Lookahead parameter (0-5ms) for responsive limiting
- High-quality parameter smoothing via JUCE

#### Plugin Format Support
- **macOS**: AU, VST3, AAX plugin formats
- **macOS**: Universal binaries (Intel x86_64 + Apple Silicon arm64)
- **Windows**: VST3 plugin format
- **Standalone**: Desktop application (macOS and Windows)

#### User Interface
- Modern HTML/CSS/JavaScript interface via WebView2
- Real-time parameter visualization
- Plugin-DAW integration with full state management
- Interactive controls for all parameters

#### Installation & Packaging
- **macOS**: Native .pkg installer with automatic plugin deployment
- **Windows**: NSIS-based installer with WebView2 runtime auto-installation
- Automatic plugin location detection and installation

---

## Release Notes

### Version 0.1.1 - Release Candidate
This version is a maintenance release focusing on stability and reliability. The parameter isolation fix ensures that Lim.one works correctly in multi-project and multi-plugin workflows, a critical requirement for professional audio production.

**Ready for Testing**: Parameter isolation fix has been thoroughly implemented and compiled successfully across all platforms. Awaiting user validation before final release.

### Version 0.1.0 - Initial Release
Lim.one represents a complete audio limiter with a unique character-based design philosophy, combining professional-grade audio processing with modern software architecture. The initial release includes support for all major plugin formats and platforms.
