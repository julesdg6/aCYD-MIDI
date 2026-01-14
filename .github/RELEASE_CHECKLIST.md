# Release Checklist Template

Use this checklist when preparing a new release.

## Pre-Release

- [ ] All features for this release are complete and tested
- [ ] All pull requests for this release are merged to main
- [ ] Code builds successfully on all platforms
- [ ] Documentation is updated to reflect new features
- [ ] Breaking changes are documented

## Version Update

- [ ] Update version in `common_definitions.h`:
  ```cpp
  #define ACYD_MIDI_VERSION "X.Y.Z"
  ```
- [ ] Add new section to `CHANGELOG.md` with:
  - [ ] Version number and date
  - [ ] Added features
  - [ ] Changed features
  - [ ] Deprecated features
  - [ ] Removed features
  - [ ] Fixed bugs
  - [ ] Security updates

## Testing

- [ ] Build all configurations locally (if possible):
  - [ ] `pio run -e esp32-2432S028Rv2`
  - [ ] `pio run -e esp32-2432S028Rv2-uart2`
  - [ ] `pio run -e esp32-2432S028Rv2-uart0`
- [ ] Test firmware on actual hardware (if available)
- [ ] Verify all modes work correctly
- [ ] Test BLE MIDI connectivity
- [ ] Test hardware MIDI output (if configured)

## Release Commit

- [ ] Commit version changes:
  ```bash
  git add common_definitions.h CHANGELOG.md
  git commit -m "Prepare release vX.Y.Z"
  git push origin main
  ```

## Create Tag

- [ ] Create annotated tag:
  ```bash
  git tag -a vX.Y.Z -m "Release vX.Y.Z - Brief description"
  ```
- [ ] Push tag to trigger release workflow:
  ```bash
  git push origin vX.Y.Z
  ```

## Monitor Release

- [ ] Watch GitHub Actions workflow: https://github.com/julesdg6/aCYD-MIDI/actions
- [ ] Verify all build jobs succeed
- [ ] Verify release is created automatically
- [ ] Verify firmware binaries are attached to release

## Post-Release

- [ ] Edit GitHub release to add detailed release notes
  - [ ] Copy content from CHANGELOG.md
  - [ ] Add installation instructions
  - [ ] Add upgrade notes if applicable
  - [ ] Add known issues if any
- [ ] Test download and installation of release artifacts
- [ ] Announce release (if applicable):
  - [ ] GitHub Discussions
  - [ ] Social media
  - [ ] Project website

## Rollback (if needed)

If something goes wrong:

- [ ] Delete the tag locally: `git tag -d vX.Y.Z`
- [ ] Delete the tag remotely: `git push origin :refs/tags/vX.Y.Z`
- [ ] Delete the GitHub release via web interface
- [ ] Fix issues
- [ ] Increment patch version
- [ ] Start release process again

## Version Numbering Guide

Following [Semantic Versioning](https://semver.org/):

- **MAJOR** (X.0.0) - Incompatible API changes, major refactors
- **MINOR** (0.X.0) - New features, backwards compatible
- **PATCH** (0.0.X) - Bug fixes, backwards compatible

Examples:
- New MIDI mode added: Minor version bump (1.0.0 → 1.1.0)
- Bug fix in sequencer: Patch version bump (1.1.0 → 1.1.1)
- Complete UI redesign breaking old configs: Major version bump (1.1.1 → 2.0.0)
