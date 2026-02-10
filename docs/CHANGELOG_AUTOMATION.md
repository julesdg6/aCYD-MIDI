# Changelog Automation for Releases

## Overview

This document describes the automatic changelog extraction feature implemented for GitHub releases.

## Implementation

The GitHub Actions release workflow (`build-esp32.yml`) now automatically extracts changelog content from `docs/CHANGELOG.md` and includes it in the release body when creating a new release.

## How It Works

When a version tag (e.g., `v0.0.3`) is pushed to the repository:

1. **Version Extraction**: The workflow extracts the version number from the Git tag
   - Example: `refs/tags/v0.0.3` → `0.0.3`

2. **Changelog Parsing**: An AWK script searches `docs/CHANGELOG.md` for the matching version section
   - Looks for a header matching `## [0.0.3]`
   - Extracts all content from that header until the next version header
   - Stops at the next `## [` pattern

3. **Release Creation**: The extracted changelog content is included as the release body
  - If no matching version is found, a fallback message is used with a link to docs/CHANGELOG.md

## Workflow Changes

### Added Step: Checkout Repository
```yaml
- name: Checkout repository
  uses: actions/checkout@v4
```

This step was added to the release job to ensure docs/CHANGELOG.md is available.

### Added Step: Extract Changelog
```yaml
- name: Extract changelog for this release
  id: changelog
  run: |
    VERSION=${GITHUB_REF#refs/tags/v}
    
    CHANGELOG_CONTENT=$(awk -v version="$VERSION" '
      /^## \[/ {
        if (found) exit
        if ($0 ~ "\\[" version "\\]") {
          found=1
          next
        }
      }
      found { print }
    ' docs/CHANGELOG.md)
    
    if [ -z "$CHANGELOG_CONTENT" ]; then
      CHANGELOG_CONTENT="See [docs/CHANGELOG.md](https://github.com/${{ github.repository }}/blob/${{ github.ref_name }}/docs/CHANGELOG.md) for details."
    fi
    
    {
      echo 'content<<EOF'
      echo "$CHANGELOG_CONTENT"
      echo 'EOF'
    } >> "$GITHUB_OUTPUT"
```

### Modified Step: Create GitHub Release
```yaml
- name: Create GitHub Release
  uses: softprops/action-gh-release@v2
  with:
    files: release-files/*
    body: ${{ steps.changelog.outputs.content }}
```

The `body` parameter now references the extracted changelog content.

## docs/CHANGELOG.md Format Requirements

For automatic extraction to work correctly, `docs/CHANGELOG.md` must follow this format:

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Added
- New feature 1
- New feature 2

### Changed
- Modified feature 1

### Fixed
- Bug fix 1

### Removed
- Deprecated feature 1
```

**Key Requirements:**
- Version header must use the format: `## [X.Y.Z] - YYYY-MM-DD`
- Version number in header must match the Git tag (without the `v` prefix)
- Content is extracted from the version header until the next `## [` header

## Example

For tag `v0.0.3`, the workflow will extract this section from docs/CHANGELOG.md:

```markdown
## [0.0.3] - 2026-02-05

### Added
- ESP32-4832S040R (4" 480x320 resistive touch) board support with UART variants (#98)
- ESP32-S3 headless USB MIDI dongle support (#98)
...

### Fixed
- GitHub release workflow now properly includes firmware binaries (this release)
- MIDI clock timing issues with uClock library integration (#86)
...
```

This content becomes the release body, visible to users when they view the release on GitHub.

## Testing

The changelog extraction logic was tested with:

1. **Positive Test**: Extracting an existing version (0.0.3)
   - ✅ Successfully extracted all content between version headers

2. **Negative Test**: Attempting to extract a non-existent version (9.9.9)
   - ✅ Correctly fell back to default message

3. **Workflow Validation**: 
   - ✅ YAML syntax validated with Python's yaml parser
   - ✅ Workflow validated with actionlint (no errors)

## Benefits

1. **Consistency**: Release notes always match the CHANGELOG
2. **Automation**: No manual copy-paste needed when creating releases
3. **Documentation**: Single source of truth for version history
4. **Transparency**: Users can see what changed in each release directly on GitHub

## Fallback Behavior

If the version is not found in docs/CHANGELOG.md, the release will include:

```
See [docs/CHANGELOG.md](https://github.com/julesdg6/aCYD-MIDI/blob/vX.Y.Z/docs/CHANGELOG.md) for details.
```

This ensures releases are never created without any description, even if the changelog is missing.

## Related Documentation

- `docs/RELEASE.md` - Complete release process documentation
- `docs/CHANGELOG.md` - Project changelog following Keep a Changelog format
- `.github/workflows/build-esp32.yml` - The workflow implementing this feature
