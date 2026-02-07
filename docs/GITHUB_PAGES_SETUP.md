# GitHub Pages Setup Instructions

This repository is now configured to publish web pages via GitHub Pages using the `/docs` folder.

## How to Enable GitHub Pages

After merging this PR, follow these steps to enable GitHub Pages:

1. **Go to your repository on GitHub**: https://github.com/julesdg6/aCYD-MIDI

2. **Navigate to Settings**:
   - Click on the "Settings" tab in the repository menu

3. **Open Pages Settings**:
   - In the left sidebar, click on "Pages" (under "Code and automation")

4. **Configure GitHub Pages**:
   - Under "Build and deployment":
     - **Source**: Select "Deploy from a branch"
     - **Branch**: Select `main`
     - **Folder**: Select `/docs`
   - Click "Save"

5. **Wait for deployment**:
   - GitHub will automatically build and deploy your site
   - This usually takes 1-2 minutes
   - You'll see a green checkmark when it's ready

6. **Access your site**:
   - Your site will be available at: **https://julesdg6.github.io/aCYD-MIDI/**
   - The main landing page: https://julesdg6.github.io/aCYD-MIDI/
   - Web installer: https://julesdg6.github.io/aCYD-MIDI/flash.html
   - Debug console: https://julesdg6.github.io/aCYD-MIDI/debug-console/

## What's Published

The following content is now available via GitHub Pages:

### Main Landing Page (`/docs/index.html`)
- Welcome page with project overview
- Links to Web Installer, Debug Console, and Documentation
- Styled with purple gradient theme matching the project aesthetic

### Web Installer (`/docs/flash.html`)
- ESP Web Tools-based firmware flasher
- Browser-based installation (no software required)
- Supports all board variants (CYD, ESP32-4832S035, ESP32-S3, etc.)
- Manifest paths updated to work from `/docs` folder

### Debug Console (`/debug-console/`)
- Web-based debugging tool
- BLE MIDI and Web Serial monitoring
- Live control panel and logging
- Already in correct location (no changes needed)

## File Structure

```
/
├── docs/
│   ├── index.html              ← Main landing page
│   ├── flash.html              ← Web installer
│   ├── manifests/              ← ESP Web Tools manifests (COPIED from root)
│   │   └── *.json
│   ├── GITHUB_PAGES_SETUP.md   ← This file
│   └── *.md                    ← Documentation files
├── debug-console/
│   ├── index.html              ← Debug console (already exists)
│   └── src/                    ← Debug console source
├── manifests/                  ← ESP Web Tools manifests (SOURCE)
│   └── *.json
└── flash.html                  ← Original (kept for backwards compatibility)
```

## Updating Content

### To update the landing page:
Edit `/docs/index.html`

### To update the web installer:
Edit `/docs/flash.html` (and optionally `/flash.html` for backwards compatibility)

### To update the debug console:
Edit files in `/debug-console/`

### To add new documentation pages:
Add markdown (`.md`) or HTML files to `/docs/`
- Markdown files can be linked using GitHub blob URLs
- HTML files will be served directly by GitHub Pages

## Troubleshooting

### "404 Not Found" errors
- Ensure the entry file is named `index.html` in the `/docs` folder
- Check that all paths are relative (e.g., `./manifests/` for files in `/docs/manifests/`)
- Wait a few minutes after deployment for DNS propagation

### Web Installer not working
- Verify manifest paths point to `./manifests/*.json` (relative to `/docs/flash.html`)
- Check that manifest files exist in `/docs/manifests/` folder
- Ensure ESP Web Tools script is loaded properly
- **Note**: GitHub Pages serves only from the `/docs` folder, so manifests must be copied there

### Debug Console not loading
- The debug console is at `/debug-console/` (outside `/docs/`)
- GitHub Pages serves the entire repository at the root
- Links should use `../debug-console/` from `/docs/`

## Custom Domain (Optional)

To use a custom domain:
1. Add a `CNAME` file to `/docs/` with your domain name
2. Configure DNS settings for your domain
3. Enable "Enforce HTTPS" in Pages settings

## Notes

- **Relative paths**: All paths are relative to work under the `/aCYD-MIDI/` subdirectory
- **Original files**: The original `/flash.html` is kept for backwards compatibility
- **Automatic builds**: GitHub Pages rebuilds automatically on every push to `main`
- **Branch protection**: Consider enabling branch protection for the `main` branch

## Questions or Issues?

If you encounter any problems with GitHub Pages setup:
1. Check the "Actions" tab for deployment logs
2. Review the Pages settings in repository Settings
3. Verify all file paths are correct and relative
4. Wait a few minutes for changes to propagate

---

**Ready to publish!** Once you merge this PR and configure the Pages settings as described above, your site will be live at https://julesdg6.github.io/aCYD-MIDI/
