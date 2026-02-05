# Discord Webhook Release Notification Not Working

## Description
The Discord webhook notification for releases is not functioning as expected. When a new release is published, the notification should be sent to Discord via the configured webhook, but this is currently not happening.

## Current Configuration
- **Workflow File**: `.github/workflows/discord-release.yml`
- **Trigger**: `on.release.types: [published]`
- **Action Used**: `teneplaysofficial/releasify-action@v1`
- **Required Secret**: `DISCORD_WEBHOOK`

## How It Should Work
1. When a GitHub release is published (tagged and created successfully)
2. The `discord-release.yml` workflow should trigger automatically
3. The workflow should send a formatted embed message to Discord containing:
   - Release title (e.g., "🚀 New Release!")
   - Release version tag
   - Release notes/body
   - Download links for firmware assets
   - Footer: "Published by CI"

## Why It Might Not Be Working

### Possible Causes:
1. **Missing or Invalid Discord Webhook Secret**
   - The `DISCORD_WEBHOOK` secret may not be configured in the repository settings
   - The webhook URL may be invalid or expired
   - **Location**: Repository → Settings → Secrets and Variables → Actions

2. **Workflow Not Triggering**
   - The workflow only triggers on `release.types: [published]`
   - If releases are created in "draft" mode or as "pre-release", this trigger might not fire
   - The previous permission issue preventing release creation would also prevent this workflow from ever triggering

3. **Action Version or Configuration Issues**
   - The action reference was recently fixed from `<commit-sha>` placeholder to `@v1`
   - May need to verify the action is compatible with the current GitHub Actions environment

4. **Webhook Permission Issues**
   - Discord server settings might have restrictions on webhooks
   - The webhook might have been deleted or disabled from the Discord side

## Steps to Diagnose

1. **Verify Discord Webhook Secret**
   ```
   - Navigate to: Repository → Settings → Secrets and Variables → Actions
   - Confirm `DISCORD_WEBHOOK` secret exists and is valid
   - Test the webhook URL manually using curl or Postman
   ```

2. **Check Workflow Run History**
   ```
   - Go to: Repository → Actions → "Notify Discord on Release"
   - Check if the workflow has ever been triggered
   - Review logs if it has run
   ```

3. **Test Manual Trigger** (if supported)
   ```
   - Add `workflow_dispatch` trigger temporarily for testing
   - Manually trigger the workflow to see if it executes
   ```

4. **Verify Discord Webhook**
   ```
   - Go to Discord Server Settings → Integrations → Webhooks
   - Confirm the webhook still exists and points to the correct channel
   - Test sending a message to the webhook directly
   ```

## Recommended Next Steps

1. **Immediate Actions:**
   - Verify `DISCORD_WEBHOOK` secret is configured correctly in repository settings
   - Check Discord server to confirm webhook still exists
   - Review workflow run history for any error messages

2. **Testing:**
   - After fixing the release creation permission issue (from the main issue), create a test release
   - Monitor the "Notify Discord on Release" workflow execution
   - Check Discord channel for the notification

3. **If Still Not Working:**
   - Add debug logging to the workflow
   - Consider adding a manual trigger option for testing
   - Verify the webhook URL by testing it directly with a curl command

## Related Issues
- This issue is related to the main release creation problem (permission 403 error)
- The Discord notification workflow can only trigger after releases are successfully created
- Fix for the main permission issue should be implemented first

## Priority
**Medium** - This is a nice-to-have feature for notifications, but not critical for release functionality. Should be addressed after the main release creation issue is resolved.
