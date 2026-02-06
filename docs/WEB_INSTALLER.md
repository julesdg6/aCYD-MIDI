# Web Installer Setup Guide

This guide explains how to enable and use the ESP Web Tools-based firmware installer for aCYD MIDI.

## What is the Web Installer?

The web installer allows users to flash aCYD MIDI firmware directly from their browser without installing any software. It uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/) and the Web Serial API.

## How It Works

1. **User visits the web page** hosted on GitHub Pages
2. **Selects their board model** from the available options
3. **Connects device via USB** and grants browser access to the serial port
4. **Firmware is flashed** directly to the ESP32 over USB
5. **Device restarts** with new firmware ready to use

## Enabling GitHub Pages

GitHub Pages must be enabled for the web installer to work:

1. Go to repository **Settings**
2. Navigate to **Pages** section (under "Code and automation")
3. Under **Source**, select:
   - Branch: `gh-pages`
   - Folder: `/ (root)`
4. Click **Save**
5. Wait a few minutes for GitHub Pages to build and deploy
6. The web installer will be available at: `https://julesdg6.github.io/aCYD-MIDI/flash.html`

## File Structure

The web installer consists of:

```
flash.html                    # Main web installer interface
manifests/                    # Manifest files for each board
├── esp32-2432S028Rv2.json
├── esp32-2432S028Rv2-uart0.json
├── esp32-2432S028Rv2-uart2.json
├── esp32-4832S035C.json
├── esp32-4832S035C-uart0.json
├── esp32-4832S035C-uart2.json
├── esp32-4832S035R.json
├── esp32-4832S035R-uart0.json
├── esp32-4832S035R-uart2.json
├── esp32-4832S040R.json
├── esp32-4832S040R-uart0.json
├── esp32-4832S040R-uart2.json
├── esp32-headless-midi-master.json
├── esp32s3-headless.json
├── firmware-*.bin            # Firmware binaries (added by CI)
├── bootloader.bin            # ESP32 bootloader (added by CI)
├── bootloader-s3.bin         # ESP32-S3 bootloader (added by CI)
├── partitions.bin            # ESP32 partition table (added by CI)
├── partitions-s3.bin         # ESP32-S3 partition table (added by CI)
└── boot_app0.bin             # Boot app (added by CI)
```

## How the Build System Works

### Automated Deployment

When a new release tag is pushed (e.g., `v1.0.0`):

1. **Build workflow runs** for all board configurations
2. **Collects firmware files:**
   - `firmware-{board}.bin` - Main firmware
   - `bootloader.bin` - ESP32 bootloader
   - `partitions.bin` - Partition table
   - `boot_app0.bin` - Boot application
3. **Prepares web installer:**
   - Copies `flash.html` and manifests
   - Adds all firmware binaries
   - Adds bootloader and partition files
4. **Deploys to GitHub Pages:**
   - Pushes everything to `gh-pages` branch
   - GitHub Pages automatically serves the files

### Manifest Files

Each manifest file (`.json`) defines the flashing parameters for a specific board:

```json
{
  "name": "aCYD MIDI - ESP32-2432S028Rv2",
  "version": "latest",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "bootloader.bin", "offset": 4096 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "firmware-esp32-2432S028Rv2.bin", "offset": 65536 }
      ]
    }
  ]
}
```

**Offset values:**
- `4096` (0x1000) - Bootloader location
- `32768` (0x8000) - Partition table location
- `57344` (0xe000) - Boot app location
- `65536` (0x10000) - Firmware location

These are standard ESP32 memory addresses and should not be changed unless you modify the partition scheme.

## Browser Requirements

The Web Serial API has specific requirements:

- **Supported Browsers:**
  - Google Chrome (version 89+)
  - Microsoft Edge (version 89+)
  - Opera (version 75+)
  
- **Not Supported:**
  - Firefox (no Web Serial API support)
  - Safari (no Web Serial API support)
  - Mobile browsers (limited/no support)

- **Connection Requirements:**
  - HTTPS connection (GitHub Pages provides this)
  - USB cable connection to ESP32
  - User permission to access serial port

## Troubleshooting

### Web Installer Not Loading

1. Verify GitHub Pages is enabled (Settings → Pages)
2. Check that `gh-pages` branch exists
3. Wait a few minutes after enabling GitHub Pages
4. Try accessing `https://julesdg6.github.io/aCYD-MIDI/` to verify pages is working

### Flashing Fails

1. **"No port selected"**
   - Make sure device is connected via USB
   - Try a different USB cable
   - Check that ESP32 drivers are installed

2. **"Failed to connect"**
   - Close any serial monitors or other programs using the port
   - Press and hold BOOT button while clicking install
   - Try a different USB port

3. **"File not found" errors**
   - Wait for a release to be published first
   - Check that bootloader/partition files are in `manifests/` folder
   - Verify GitHub Actions workflow completed successfully

### Browser Not Supported

1. Use Chrome, Edge, or Opera
2. Update browser to latest version
3. As a fallback, use Option B (command-line flashing)

## Updating the Web Installer

### Adding a New Board

1. **Create manifest file** in `manifests/` directory:
   ```json
   {
     "name": "aCYD MIDI - Your Board Name",
     "version": "latest",
     "new_install_prompt_erase": true,
     "builds": [
       {
         "chipFamily": "ESP32",
         "parts": [
           { "path": "bootloader.bin", "offset": 4096 },
           { "path": "partitions.bin", "offset": 32768 },
           { "path": "boot_app0.bin", "offset": 57344 },
           { "path": "firmware-your-board.bin", "offset": 65536 }
         ]
       }
     ]
   }
   ```

2. **Add board section to `flash.html`:**
   ```html
   <div class="board-section">
       <h2>Your Board Name</h2>
       <p class="board-description">Description here</p>
       
       <esp-web-install-button manifest="./manifests/your-board.json">
           <button slot="activate">Install Your Board</button>
       </esp-web-install-button>
   </div>
   ```

3. **Add environment to build workflow** in `.github/workflows/build-esp32.yml`

4. **Commit and create a release tag** - GitHub Actions will handle the rest

### Changing Styling

Edit the `<style>` section in `flash.html`. The page uses:
- Gradient background: `linear-gradient(135deg, #667eea 0%, #764ba2 100%)`
- Primary color: `#667eea`
- Card-based layout with rounded corners
- Responsive design

### Testing Changes Locally

You can test the web installer locally:

1. Run a local web server in the repository root:
   ```bash
   python -m http.server 8000
   ```

2. Visit `http://localhost:8000/flash.html`

3. Note: You'll need actual firmware files in `manifests/` to test flashing

## Security Considerations

- **HTTPS required**: Web Serial API only works over HTTPS
- **User consent**: Browser prompts user to select serial port
- **File integrity**: Users should only install from official repository pages
- **No credentials**: Web installer doesn't handle WiFi credentials (use WiFiManager)

## Resources

- [ESP Web Tools Documentation](https://esphome.github.io/esp-web-tools/)
- [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
- [ESP-Web-Tools Tutorial](https://github.com/witnessmenow/ESP-Web-Tools-Tutorial)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)

## Support

If users have issues with the web installer:

1. Direct them to try Option B (command-line flashing)
2. Check browser compatibility
3. Verify drivers are installed
4. Ensure they're using the official repository page
5. Check GitHub Pages deployment status
