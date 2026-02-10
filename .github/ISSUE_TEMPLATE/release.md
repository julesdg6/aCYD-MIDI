---
name: Release Planning
about: Template for planning a new release
title: 'Release vX.Y.Z'
labels: release
assignees: ''
---

## Release Information

**Version**: vX.Y.Z
**Target Date**: YYYY-MM-DD
**Type**: Major / Minor / Patch

## Features/Changes Included

<!-- List all features, improvements, and fixes to be included in this release -->

- [ ] Feature 1
- [ ] Feature 2
- [ ] Bug fix 1

## Documentation Updates Needed

- [ ] Update README.md
- [ ] Update docs/CHANGELOG.md
- [ ] Update relevant docs in `/docs`
- [ ] Update version in `common_definitions.h`

## Testing Requirements

- [ ] Build test on all configurations
- [ ] Test on physical hardware
- [ ] Test BLE MIDI connectivity
- [ ] Test hardware MIDI output
- [ ] Test all modes
- [ ] Test remote display feature
- [ ] Test screenshot capture

## Pre-Release Checklist

- [ ] All PRs merged to main
- [ ] Version updated in code
- [ ] docs/CHANGELOG.md updated
- [ ] All tests passing
- [ ] Documentation complete
- [ ] Breaking changes documented

## Release Process

See [docs/RELEASE_CHECKLIST.md](../docs/RELEASE_CHECKLIST.md) for detailed steps.

## Post-Release Tasks

- [ ] Verify GitHub release created
- [ ] Verify firmware artifacts attached
- [ ] Edit release notes
- [ ] Test firmware downloads
- [ ] Announce release

## Notes

<!-- Any additional notes, considerations, or concerns -->
