# Discord Release Notifications

This document explains how Discord release notifications work and how to troubleshoot them.

## How It Works

When you create a new release on GitHub (e.g., v0.1.0), the `discord-release.yml` workflow automatically:

1. Extracts release information (name, tag, description, URL)
2. Truncates the description if too long (Discord limit: 2000 characters)
3. Creates a formatted Discord embed with:
   - Title: "🚀 New Release: [name]"
   - Description: Release notes
   - URL: Link to GitHub release
   - Timestamp: Release publish date
4. Sends the embed to your Discord webhook

## Recent Fix (v0.1.0)

The Discord notification for v0.1.0 **failed** due to a shell syntax error.

**Problem**: The release body contained special characters (parentheses in URLs) that caused shell parsing errors:
```
[Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
```

**Solution**: Refactored the workflow to use `jq` with `--arg` parameters, which properly escapes all JSON content.

## Manually Trigger Notification for v0.1.0

To resend the Discord notification for v0.1.0:

### Via GitHub CLI

```bash
gh workflow run discord-release.yml
```

### Via GitHub Web UI

1. Go to: https://github.com/julesdg6/aCYD-MIDI/actions/workflows/discord-release.yml
2. Click **"Run workflow"** button (top right)
3. Select branch: **main**
4. Click **"Run workflow"**

**Note**: The workflow will use the information from the **most recent release**, not necessarily v0.1.0. To specifically notify about v0.1.0, you would need to edit and re-publish that release, which will trigger the workflow automatically.

### Re-publishing a Release

If you want to specifically resend the notification for v0.1.0:

1. Go to: https://github.com/julesdg6/aCYD-MIDI/releases/tag/v0.1.0
2. Click **"Edit release"**
3. Make a small change (e.g., add a space to the description)
4. Click **"Update release"**

This will trigger the Discord notification workflow for v0.1.0.

## Workflow Status

Check if the Discord notification workflow is running:

```bash
# List recent runs
gh run list --workflow=discord-release.yml --limit 5

# View specific run details
gh run view <run-id>

# View run logs
gh run view <run-id> --log
```

## Troubleshooting

### Notification Not Sent

**Check webhook is configured:**
```bash
# Verify DISCORD_WEBHOOK secret exists
gh secret list | grep DISCORD_WEBHOOK
```

If not set, add it:
```bash
gh secret set DISCORD_WEBHOOK
# Paste your Discord webhook URL when prompted
```

**Check workflow ran:**
```bash
gh run list --workflow=discord-release.yml --limit 1
```

If status is "failure", view the logs:
```bash
gh run view <run-id> --log
```

### Workflow Failed

Common issues:

1. **Missing webhook secret**: Add `DISCORD_WEBHOOK` secret in repository settings
2. **Invalid webhook URL**: Verify the webhook URL is correct
3. **Discord rate limiting**: Wait a few minutes and try again
4. **Release body too large**: The workflow truncates at 1900 characters

### Testing

To test the Discord notification without creating a real release:

1. Create a test release with tag `test-v0.0.0`
2. Mark it as a **pre-release**
3. Check Discord for the notification
4. Delete the test release after verification

## Webhook URL Format

Your Discord webhook URL should look like:
```
https://discord.com/api/webhooks/[WEBHOOK_ID]/[WEBHOOK_TOKEN]
```

**Security**: Never commit the webhook URL to the repository! Always use GitHub Secrets.

## Workflow Trigger Events

The workflow triggers on these release events:
- `published` - When a release is published (most common)
- `prereleased` - When a pre-release is published
- `released` - Alternative event for release publication

It also supports manual triggering via `workflow_dispatch`.

## Customizing Notifications

To customize the notification format, edit `.github/workflows/discord-release.yml`:

**Change embed color:**
```yaml
color: 5814783  # Current: Blue-purple
# Try:
# 3066993 - Green
# 15844367 - Gold  
# 15158332 - Red
```

**Change bot name/avatar:**
```yaml
username: "ReleaseBot"
avatar_url: "https://github.githubassets.com/images/modules/logos_page/GitHub-Mark.png"
```

**Add fields:**
```jq
{
  # ... existing fields ...
  embeds: [{
    # ... existing embed ...
    fields: [
      {
        name: "Author",
        value: "${{ github.event.release.author.login }}",
        inline: true
      }
    ]
  }]
}
```

## Getting Help

If notifications still aren't working:

1. Check workflow run logs: `gh run view <run-id> --log`
2. Verify webhook in Discord server settings
3. Test webhook manually:
   ```bash
   curl -H "Content-Type: application/json" \
        -d '{"content":"Test message"}' \
        "YOUR_WEBHOOK_URL"
   ```
4. Check Discord server has proper permissions for webhooks

## Reference

- **Discord Webhook Documentation**: https://discord.com/developers/docs/resources/webhook
- **GitHub Actions Context**: https://docs.github.com/en/actions/learn-github-actions/contexts#github-context
- **jq Manual**: https://stedolan.github.io/jq/manual/
