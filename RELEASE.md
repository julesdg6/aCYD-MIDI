# Creating a Release

This document explains how to create a new release for aCYD-MIDI.

## Release Process

The release process is automated via GitHub Actions. When you push a version tag, the workflow will:

1. Build firmware for all board configurations:
   - `esp32-2432S028Rv2` (default, UART2 for development)
   - `esp32-2432S028Rv2-uart2` (explicit UART2)
   - `esp32-2432S028Rv2-uart0` (UART0 for production)

2. Create a GitHub Release with:
   - The tag name as the release title
   - All firmware binaries (.bin and .elf files) as downloadable assets

## Steps to Create a Release

### 1. Update Version Information

Before creating a release, ensure the version is updated:

- Update `ACYD_MIDI_VERSION` in `common_definitions.h`
- Add a new entry to `CHANGELOG.md` documenting changes

### 2. Commit and Push Changes

```bash
git add common_definitions.h CHANGELOG.md
git commit -m "Prepare release v0.0.1"
git push origin main
```

### 3. Create and Push the Tag

Create a tag with the version number (must start with 'v'):

```bash
git tag -a v0.0.1 -m "Release v0.0.1 - Initial pre-release"
git push origin v0.0.1
```

### 4. Wait for GitHub Actions

The GitHub Actions workflow will automatically:
- Build all firmware variants
- Create the GitHub Release
- Attach firmware binaries

You can monitor progress at: https://github.com/julesdg6/aCYD-MIDI/actions

### 5. Edit Release Notes (Optional)

After the release is created automatically, you can edit it on GitHub to add:
- Release highlights
- Breaking changes
- Installation instructions
- Known issues

The CHANGELOG.md content can be copied into the release notes.

## Release Checklist

Before creating a release, verify:

- [ ] Version updated in `common_definitions.h`
- [ ] CHANGELOG.md updated with new version
- [ ] All changes committed and pushed to main
- [ ] Code builds successfully
- [ ] All tests pass (if applicable)
- [ ] Documentation is up to date

## Version Numbering

aCYD-MIDI follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version for incompatible API changes
- **MINOR** version for new functionality (backwards compatible)
- **PATCH** version for backwards compatible bug fixes

Format: `MAJOR.MINOR.PATCH` (e.g., `0.0.1`, `0.1.0`, `1.0.0`)

## Example: Creating v0.0.1

For the initial v0.0.1 pre-release:

```bash
# Ensure you're on the main branch and up to date
git checkout main
git pull origin main

# Create the tag
git tag -a v0.0.1 -m "Release v0.0.1 - Initial pre-release with 16 interactive modes"

# Push the tag
git push origin v0.0.1
```

The GitHub Actions workflow will handle the rest!

## Troubleshooting

**Build fails in GitHub Actions:**
- Check the Actions tab for error logs
- Verify platformio.ini configuration
- Ensure all dependencies are specified correctly

**Release not created:**
- Verify the tag starts with 'v'
- Check that you pushed the tag (not just created it locally)
- Ensure GitHub Actions has permissions to create releases

**Firmware files missing:**
- Check build logs in GitHub Actions
- Verify the build artifacts were uploaded correctly
- Check the paths in the workflow file match the actual build output
