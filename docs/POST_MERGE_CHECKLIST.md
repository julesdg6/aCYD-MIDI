# Post-Merge Checklist for Web Installer

This checklist outlines the steps needed after the PR is merged to make the web installer live.

## Step 1: Merge the PR ✅

Once the PR is reviewed and approved:
- Merge to `main` branch
- The code is now in the repository

## Step 2: Create a Release Tag 🏷️

The web installer needs firmware binaries, which are only built on release tags.

1. Go to the repository on GitHub
2. Click "Releases" → "Create a new release"
3. Create a new tag (e.g., `v1.0.0`)
4. Fill in release notes
5. Publish the release

**What happens:**
- GitHub Actions workflow triggers
- Builds firmware for all 14 board configurations
- Extracts bootloader, partition table, and boot_app0.bin files
- Creates web installer directory structure
- Copies flash.html and manifests
- Adds all firmware binaries
- Deploys everything to `gh-pages` branch

**Time:** ~10-15 minutes for all builds to complete

## Step 3: Enable GitHub Pages 🌐

After the release build completes:

1. Go to repository **Settings**
2. Navigate to **Pages** (in left sidebar under "Code and automation")
3. Under **Source**:
   - Branch: Select `gh-pages`
   - Folder: Select `/ (root)`
4. Click **Save**
5. Wait 2-3 minutes for GitHub Pages to deploy

**Result:** Web installer is now live at:
```
https://julesdg6.github.io/aCYD-MIDI/flash.html
```

## Step 4: Test the Web Installer 🧪

1. **Open in Chrome/Edge/Opera**:
   Visit: `https://julesdg6.github.io/aCYD-MIDI/flash.html`

2. **Verify the page loads**:
   - ✅ Purple gradient background
   - ✅ All board sections visible
   - ✅ Install buttons present

3. **Test with actual hardware** (if available):
   - Connect ESP32 device via USB
   - Click appropriate install button
   - Browser prompts for serial port selection
   - Firmware should flash successfully
   - Device restarts with aCYD MIDI

4. **Check console** (F12):
   - Should have no critical errors
   - ESP Web Tools should load successfully

## Step 5: Update Links (Optional) 📝

Consider adding the web installer link to:

- **README.md "Quick Start" section** - Already done! ✅
- **Repository description** - Could add link
- **Release notes** - Include web installer link
- **Social media** - Announce the easy installation method

## Troubleshooting

### GitHub Pages Not Deploying

**Check:**
1. Verify `gh-pages` branch exists
2. Check Actions tab for deployment status
3. Ensure GitHub Pages is enabled in Settings
4. Wait a few minutes - it can take time

**Fix:**
- Re-run the release workflow
- Check repository permissions
- Verify Actions have write permissions

### Firmware Files Missing

**Check:**
1. Did the release workflow complete successfully?
2. Are .bin files in the `gh-pages` branch under `manifests/`?
3. Check GitHub Actions logs for errors

**Fix:**
- Re-run the release workflow
- Check workflow YAML for syntax errors
- Verify PlatformIO builds succeeded

### Web Installer Shows Errors

**Check:**
1. Browser console (F12) for errors
2. Are manifest files valid JSON?
3. Do manifest file paths match actual files?

**Fix:**
- Validate JSON syntax
- Check file paths in manifests
- Ensure firmware files exist in manifests/ directory

### Cannot Flash Device

**Check:**
1. Using Chrome/Edge/Opera? (Not Firefox/Safari)
2. HTTPS connection? (Not HTTP or file://)
3. ESP32 drivers installed?
4. Serial port not in use by other software?

**Fix:**
- Use supported browser
- Install ESP32 USB drivers
- Close Arduino IDE, PlatformIO, serial monitors
- Try different USB cable/port

## Verification Checklist

After completing all steps, verify:

- [ ] PR merged to main
- [ ] Release tag created
- [ ] GitHub Actions completed successfully
- [ ] `gh-pages` branch exists and has content
- [ ] GitHub Pages enabled in settings
- [ ] Web installer URL works: `https://julesdg6.github.io/aCYD-MIDI/flash.html`
- [ ] Page loads without errors in console
- [ ] All board sections visible
- [ ] Install buttons present
- [ ] (Optional) Test flashing with actual hardware

## Success Criteria

✅ **Web installer is live when:**
- URL loads successfully
- Page displays all board options
- No console errors
- Firmware files accessible (check Network tab)

## Expected Timeline

- Merge PR: Immediate
- Create release: 2 minutes
- Build firmware: 10-15 minutes
- Deploy to Pages: 2-3 minutes
- **Total: ~20 minutes** from merge to live

## Support After Launch

**Direct users to:**
1. Web installer: `https://julesdg6.github.io/aCYD-MIDI/flash.html`
2. Troubleshooting: `docs/WEB_INSTALLER.md`
3. Browser requirements: Chrome, Edge, or Opera
4. Fallback: README.md Option B (command-line flashing)

**Common user issues:**
- Wrong browser → Direct to Chrome/Edge
- Driver issues → Link to ESP32 driver guide
- Port busy → Close other serial software

## Future Enhancements

Consider for future updates:
- [ ] Add firmware version selection (not just latest)
- [ ] Add WiFi credential input (requires Improv WiFi)
- [ ] Show firmware changelog on page
- [ ] Add flashing progress indicator improvements
- [ ] Support for custom firmware uploads

## Notes

- The web installer will automatically update with each new release
- No manual file copying needed - GitHub Actions handles it
- Users always get the latest firmware
- Works across all platforms (Windows, Mac, Linux, ChromeOS)

---

**Questions?** See `docs/WEB_INSTALLER.md` for detailed information.
