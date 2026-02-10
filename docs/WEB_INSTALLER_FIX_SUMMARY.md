# Web Installer Fix Summary

## Issues Fixed

This PR addresses the web installer issues reported in the GitHub Pages deployment:

### 1. MIME Type Error ✅
**Problem:** Browser console showed:
```
Failed to load module script: Expected a JavaScript-or-Wasm module script 
but the server responded with a MIME type of "video/mp2t"
```

**Cause:** The ESP Web Tools script URL had an incorrect `?module` query parameter:
```html
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
```

**Fix:** Removed the `?module` query parameter from both `flash.html` and `docs/flash.html`:
```html
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js"></script>
```

### 2. Missing Firmware Binaries (404 Errors) ✅
**Problem:** Browser console showed multiple 404 errors:
```
GET https://julesdg6.github.io/aCYD-MIDI/manifests/partitions.bin 404
GET https://julesdg6.github.io/aCYD-MIDI/manifests/bootloader.bin 404
GET https://julesdg6.github.io/aCYD-MIDI/manifests/boot_app0.bin 404
GET https://julesdg6.github.io/aCYD-MIDI/manifests/firmware-*.bin 404
```

**Cause:** The GitHub Pages deployment workflow only ran on tagged releases (`if: startsWith(github.ref, 'refs/tags/')`), not on regular pushes to main. Since no release tag had been created yet, the firmware binaries were never deployed to the `gh-pages` branch.

**Fix:** Added a new `deploy-pages` job that runs on every push to the `main` branch. This job:
- Runs after the build job completes
- Downloads all firmware artifacts from the build
- Prepares the web installer directory structure
- Copies firmware binaries to `web-installer/manifests/`
- Deploys everything to the `gh-pages` branch

## What Happens Next

When this PR is merged to `main`:

1. **The workflow will automatically run** and:
   - Build all 14 firmware variants
   - Deploy the web installer files to the `gh-pages` branch
   - Deploy all firmware binaries to `gh-pages:manifests/`

2. **GitHub Pages will update** (within 1-2 minutes)

3. **The web installer will work** at:
   - https://julesdg6.github.io/aCYD-MIDI/flash.html

## Required: GitHub Pages Configuration

**Important:** Ensure GitHub Pages is configured correctly:

1. Go to **Settings → Pages** in your repository
2. Under "Build and deployment":
   - **Source:** Deploy from a branch
   - **Branch:** `gh-pages`
   - **Folder:** `/ (root)`
3. Click **Save**

**Note:** If GitHub Pages is configured to deploy from `/docs` on `main`, it will NOT work because the firmware binaries are deployed to the `gh-pages` branch, not to `/docs`.

## Testing the Fix

After the PR is merged and the workflow completes:

1. Wait 1-2 minutes for GitHub Pages to update
2. Visit https://julesdg6.github.io/aCYD-MIDI/flash.html
3. Connect your ESP32 device via USB
4. Click an install button for your board
5. Select the serial port when prompted
6. The firmware should flash successfully

## Verification Commands

To verify the deployment worked:

```bash
# Check that gh-pages branch exists
git fetch origin gh-pages
git ls-remote origin gh-pages

# View deployed firmware files
git show origin/gh-pages:manifests/ | grep "\.bin$"
```

Expected output should include:
- `bootloader.bin`
- `boot_app0.bin`
- `partitions.bin`
- `firmware-esp32-2432S028Rv2.bin`
- `firmware-esp32-4832S035C.bin`
- `firmware-esp32-4832S040R.bin`
- etc. (all 14 firmware variants)

## Files Changed

1. **`flash.html`** - Fixed script URL
2. **`docs/flash.html`** - Fixed script URL
3. **`.github/workflows/build-esp32.yml`** - Added `deploy-pages` job

## Why This Works

### Before:
- Firmware binaries only deployed on **tagged releases**
- No binaries available for web installer to reference
- 404 errors when trying to load firmware

### After:
- Firmware binaries deployed on **every push to main**
- Binaries always available at `gh-pages:manifests/*.bin`
- Web installer can load and flash firmware successfully

## Release Workflow Still Works

The existing `release` job is unchanged and still:
- Creates GitHub Releases with firmware files
- Deploys to GitHub Pages on tagged releases
- Generates release notes from docs/CHANGELOG.md

Now there are **two paths** to deployment:
1. **Continuous deployment:** Every push to `main` → web installer updated
2. **Release deployment:** Tagged releases → GitHub Release created + web installer updated

## Troubleshooting

If the web installer still doesn't work after merging:

1. **Check GitHub Pages settings** (see above)
2. **Wait for workflow to complete:**
   - Go to Actions tab
   - Wait for "Build ESP32 firmware" workflow to finish
   - Check that "deploy-pages" job succeeded
3. **Clear browser cache** or use Incognito mode
4. **Check browser console** (F12 → Console) for any remaining errors
5. **Verify deployment:**
   ```bash
   curl -I https://julesdg6.github.io/aCYD-MIDI/manifests/bootloader.bin
   ```
   Should return `200 OK`, not `404`

## Summary

This fix ensures that:
- ✅ The MIME type error is resolved
- ✅ Firmware binaries are deployed automatically on every main branch push
- ✅ The web installer works without requiring manual release creation
- ✅ Users can flash firmware directly from their browser
- ✅ The workflow is triggered automatically on merge

The web installer should now work immediately after this PR is merged and the workflow completes.
