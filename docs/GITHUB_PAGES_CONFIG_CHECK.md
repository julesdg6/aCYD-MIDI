# CRITICAL: GitHub Pages Configuration Check

## ⚠️ ACTION REQUIRED AFTER MERGING

After this PR is merged, you **MUST** verify your GitHub Pages settings are correct.

## The Problem

There are **two different** GitHub Pages configurations in this repository's documentation:

1. **`docs/GITHUB_PAGES_SETUP.md`** says:
   - Branch: `main`
   - Folder: `/docs`

2. **`docs/GITHUB_PAGES_TROUBLESHOOTING.md`** says:
   - Branch: `gh-pages`
   - Folder: `/ (root)`

**Only option #2 will work** because the firmware binaries are deployed to the `gh-pages` branch by the CI workflow, NOT to the `/docs` folder.

## Correct Configuration

1. Go to your repository: https://github.com/julesdg6/aCYD-MIDI

2. Click **Settings** → **Pages**

3. Under "Build and deployment":
   - **Source:** Deploy from a branch
   - **Branch:** `gh-pages` ← **MUST be gh-pages, NOT main**
   - **Folder:** `/ (root)` ← **MUST be root**

4. Click **Save**

5. Wait 1-2 minutes for the site to deploy

## How to Verify

After the workflow runs (check the Actions tab), verify the deployment:

```bash
# Check that gh-pages branch exists
git ls-remote origin gh-pages

# Check firmware files are deployed
curl -I https://julesdg6.github.io/aCYD-MIDI/manifests/bootloader.bin
```

Should return `HTTP/2 200` (not 404).

## Why This Matters

The web installer loads firmware from:
```
https://julesdg6.github.io/aCYD-MIDI/manifests/bootloader.bin
https://julesdg6.github.io/aCYD-MIDI/manifests/partitions.bin
https://julesdg6.github.io/aCYD-MIDI/manifests/boot_app0.bin
https://julesdg6.github.io/aCYD-MIDI/manifests/firmware-*.bin
```

These files are deployed to the `gh-pages` branch by the GitHub Actions workflow.

If GitHub Pages is configured to serve from `/docs` on `main`:
- ❌ The HTML files will load (they're in `/docs`)
- ❌ The firmware files will 404 (they're on `gh-pages`)
- ❌ The web installer will fail

If GitHub Pages is configured to serve from `gh-pages`:
- ✅ Everything is on the same branch
- ✅ All files load correctly
- ✅ The web installer works

## Summary

**After merging this PR:**
1. ✅ Merge the PR
2. ⚠️ **IMMEDIATELY** check Settings → Pages
3. ⚠️ **VERIFY** it's set to `gh-pages` branch
4. ⚠️ **CHANGE** it if it says `main` or `/docs`
5. ✅ Wait for deployment to complete
6. ✅ Test the web installer

**DO NOT SKIP STEP 2-4** or the web installer will still show 404 errors!
