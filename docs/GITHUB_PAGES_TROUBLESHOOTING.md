# GitHub Pages Troubleshooting Guide

This document helps you set up and troubleshoot the aCYD-MIDI web pages hosted on GitHub Pages.

## Quick Links

- **Web Installer**: https://julesdg6.github.io/aCYD-MIDI/flash.html
- **Debug Console**: https://julesdg6.github.io/aCYD-MIDI/debug-console/
- **Landing Page**: https://julesdg6.github.io/aCYD-MIDI/

## Initial Setup

### Enable GitHub Pages

GitHub Pages must be manually enabled in repository settings:

1. Go to **Settings** → **Pages** in your GitHub repository
2. Under "Build and deployment":
   - Source: **Deploy from a branch**
   - Branch: **gh-pages**
   - Folder: **/ (root)**
3. Click **Save**
4. Wait 1-2 minutes for the site to deploy
5. GitHub will show the URL where your site is published

### Verify Deployment

Check that GitHub Pages is working:

```bash
# Check if gh-pages branch exists
git ls-remote origin gh-pages

# View deployed files
git ls-tree -r origin/gh-pages
```

Expected files on gh-pages branch:
- `/debug-console/index.html` - Debug console (built)
- `/debug-console/assets/` - JS and CSS bundles
- `/flash.html` - Web installer
- `/index.html` - Landing page  
- `/manifests/*.bin` - Firmware binaries
- `/manifests/*.json` - Manifest files

## Common Issues

### Issue 1: Debug Console Shows 404 for main.ts

**Symptoms:**
- Browser console error: "Failed to load resource: main.ts 404"
- Debug console page doesn't load properly

**Causes:**
1. GitHub Pages not enabled
2. Browser showing cached development version
3. Accessing source repository files instead of deployed site

**Solutions:**

**A. Enable GitHub Pages** (if not already done - see above)

**B. Clear browser cache:**
- Chrome: DevTools → Network tab → Disable cache (while DevTools open)
- Firefox: DevTools → Network tab → Disable HTTP Cache
- Or use Incognito/Private browsing mode

**C. Verify you're accessing the correct URL:**
- ✅ Correct: `https://julesdg6.github.io/aCYD-MIDI/debug-console/`
- ❌ Wrong: `https://github.com/julesdg6/aCYD-MIDI/debug-console/` (source files)

**D. Force reload:**
- Windows/Linux: `Ctrl + Shift + R`
- Mac: `Cmd + Shift + R`

### Issue 2: Flash Page Shows "Installation failed"

**Symptoms:**
- ESP Web Tools error: "Failed to connect with the device"
- Serial port errors in console

**Causes:**
1. No ESP32 device connected (most common)
2. Browser doesn't support Web Serial API
3. Firmware binaries missing (deployment issue)

**Solutions:**

**A. Connect ESP32 device:**
- Connect ESP32 via USB cable
- Install ESP32 USB drivers if needed
- Use Chrome, Edge, or Opera browser (Web Serial API required)

**B. Check firmware binaries exist:**

```bash
# View binaries in gh-pages
git show origin/gh-pages:manifests/ | grep "\.bin$"
```

Should show files like:
- `bootloader.bin`
- `boot_app0.bin`  
- `partitions.bin`
- `firmware-esp32-2432S028Rv2.bin`
- `firmware-esp32-4832S035C.bin`
- etc.

**C. Verify binaries deployed:**
- Check latest release assets at: https://github.com/julesdg6/aCYD-MIDI/releases
- Binaries are deployed when a release tag is created (e.g., `v0.1.0`)

### Issue 3: Pages Not Updating After Workflow Runs

**Symptoms:**
- Workflow shows success but pages show old content
- Changes not reflected on live site

**Causes:**
1. Browser cache
2. GitHub Pages CDN cache
3. Workflow didn't actually deploy

**Solutions:**

**A. Check workflow ran successfully:**

```bash
# View recent workflow runs
gh run list --workflow=deploy-debug-console.yml --limit 5
gh run list --workflow=build-esp32.yml --limit 5
```

**B. Verify files were updated on gh-pages:**

```bash
# Check last commit on gh-pages
git log origin/gh-pages -1 --oneline

# Check file modification time
git show origin/gh-pages:debug-console/index.html | head -20
```

**C. Clear all caches:**
1. Hard refresh browser (Ctrl+Shift+R / Cmd+Shift+R)
2. Clear browser cache completely
3. Wait 5-10 minutes for GitHub Pages CDN to update
4. Try incognito/private mode

## Deployment Workflows

### Debug Console Deployment

Automatically deploys when changes are pushed to `debug-console/` folder:

```yaml
Trigger: Push to main (changes in debug-console/**)
Process: 
  1. Install Node.js dependencies
  2. Build with Vite (npm run build)
  3. Deploy dist/ to gh-pages:debug-console/
```

Check deployment status:
```bash
gh run list --workflow=deploy-debug-console.yml
```

### Web Installer & Firmware Deployment

Automatically deploys when a release tag is created:

```yaml
Trigger: Create release tag (e.g., v0.1.0)
Process:
  1. Build all firmware variants
  2. Create GitHub release with binaries
  3. Deploy web installer + binaries to gh-pages root
```

Check deployment status:
```bash
gh run list --workflow=build-esp32.yml --event=release
```

## Manual Deployment

If needed, you can manually trigger deployments:

### Trigger Debug Console Deployment

```bash
# Via GitHub CLI
gh workflow run deploy-debug-console.yml

# Or via GitHub web UI:
# Actions → Deploy Debug Console → Run workflow
```

### Trigger Firmware Build

```bash
# Via GitHub CLI  
gh workflow run build-esp32.yml

# Or via GitHub web UI:
# Actions → Build ESP32 firmware → Run workflow
```

## Health Check

Run these commands to verify everything is working:

```bash
# 1. Check gh-pages branch exists
git ls-remote origin gh-pages

# 2. Check debug console files exist
git ls-tree -r origin/gh-pages | grep "debug-console/index.html"
git ls-tree -r origin/gh-pages | grep "debug-console/assets"

# 3. Check firmware binaries exist  
git ls-tree -r origin/gh-pages | grep "manifests/.*\.bin$"

# 4. Check latest workflows succeeded
gh run list --workflow=deploy-debug-console.yml --limit 1
gh run list --workflow=build-esp32.yml --limit 1

# 5. Test URLs (should return 200 OK)
curl -I https://julesdg6.github.io/aCYD-MIDI/debug-console/
curl -I https://julesdg6.github.io/aCYD-MIDI/flash.html
```

## Getting Help

If issues persist after trying these solutions:

1. Check workflow logs: `gh run view <run-id> --log`
2. Verify GitHub Pages is enabled in repository settings
3. Check browser console for specific errors (F12 → Console tab)
4. Open an issue at: https://github.com/julesdg6/aCYD-MIDI/issues

Include:
- Browser name and version
- Screenshot of error
- Browser console output
- Which URL you're accessing
