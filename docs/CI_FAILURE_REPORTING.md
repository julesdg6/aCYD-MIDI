# CI Failure Reporting

## Overview

The aCYD-MIDI project includes an automated CI failure reporting system that creates GitHub issues when build failures occur. This helps track and resolve build problems efficiently.

## How It Works

When a build fails in the GitHub Actions workflow:

1. **Automatic Issue Creation**: A new GitHub issue is automatically created with:
   - Detailed failure information
   - Full log summary
   - Direct links to the failed workflow run
   - Debugging instructions
   - Labels: `ci-failure`, `automated`

2. **Parent Issue Linking** (Optional): If a parent issue number is provided, the failure issue is linked to it via a comment, creating a parent→child relationship.

## Using the Feature

### Automatic Triggering

The failure reporting runs automatically when:
- A push to `main` branch fails to build
- A pull request build fails
- A tagged release build fails

No manual intervention is needed for automatic failure detection.

### Manual Triggering with Parent Issue

To manually trigger the workflow and link failures to a specific issue:

1. Go to the **Actions** tab in the GitHub repository
2. Select the **"Build ESP32 firmware (PlatformIO)"** workflow
3. Click **"Run workflow"**
4. Enter the parent issue number in the `parent_issue` field
5. Click **"Run workflow"**

If the build fails, the created failure issue will be linked to the parent issue via a comment.

## What Gets Reported

Each failure issue includes:

### Metadata
- Workflow name and run number
- Triggering event and actor
- Branch/ref and commit SHA
- Timestamp

### Log Information
- Comprehensive failure summary
- **Full job logs from failed jobs** (fetched via GitHub API)
- Build matrix information
- Links to full workflow logs
- Links to artifacts (if any)

**Note:** The system attempts to fetch and include the complete logs from all failed jobs directly in the issue. If logs are too large (>30KB per job), they are truncated with a note to view the complete logs at the job URL.

### Debugging Guide
- Step-by-step instructions to investigate the failure
- Links to relevant resources
- Next steps for resolution

## Issue Format

### Title
```
CI Build Failure: <Workflow Name> (Run #<Run Number>)
```

### Body Structure
- **🔴 Automated CI Build Failure Report** header
- Workflow metadata (run ID, number, actor, etc.)
- **📋 Failure Summary** with context information
- **Failed Job Details** section with:
  - Job name, status, and conclusion
  - Job start/completion times
  - Direct link to job
  - **Full logs from the failed job** (up to 30KB per job)
- **🔗 Useful Links** section with direct links
- **🛠️ How to Debug** section with instructions

### Labels
- `ci-failure`: Marks the issue as a CI failure
- `automated`: Indicates automatic creation

## Parent Issue Linking

When a parent issue is provided:

1. A failure sub-issue is created as described above
2. A comment is posted on the parent issue with:
   - Link to the failure issue
   - Workflow run number
   - Direct link to the workflow run

This creates a clear parent→child relationship for tracking related issues.

## Permissions

The workflow requires the following permissions:
- `contents: write` - For creating releases and accessing repository content
- `issues: write` - For creating issues and posting comments

These permissions are configured at the workflow level.

## Example Workflow Usage

### Scenario 1: Automatic Failure Detection
```yaml
# Triggered automatically on push to main
# If build fails, creates issue automatically
# No parent issue linking
```

### Scenario 2: Manual Run with Parent Issue
```yaml
# Manually triggered via workflow_dispatch
# parent_issue: 42
# If build fails, creates issue #123 and links to #42
```

### Scenario 3: Pull Request Failure
```yaml
# Triggered by pull request
# If build fails, creates issue automatically
# No parent issue linking (unless PR workflow is customized)
```

## Troubleshooting

### Issue Not Created
- Check workflow permissions in the repository settings
- Verify the workflow completed (failure in the `build` job)
- Check the `report_failure` job logs for errors

### Parent Issue Not Linked
- Verify the parent issue number was provided correctly
- Check that the parent issue exists and is accessible
- Review the `report_failure` job logs for linking errors

### Missing Log Details
- Logs are captured from the workflow run context
- Full logs are always available via the workflow run URL
- Log summary is embedded in the issue body

## Maintenance

### Updating the Failure Report Format
Edit `.github/workflows/build-esp32.yml`, specifically:
- The `Get job logs` step for log content
- The `Create failure issue and link to parent` step for issue formatting

### Customizing Labels
Modify the `labels` array in the issue creation:
```javascript
labels: ['ci-failure', 'automated', 'your-custom-label']
```

### Adding More Context
Add additional information to the log summary in the `Get job logs` step or the issue body in the `Create failure issue and link to parent` step.

## Best Practices

1. **Review Issues Promptly**: CI failure issues indicate real problems that should be addressed quickly
2. **Close When Fixed**: Close failure issues once the underlying problem is resolved
3. **Link Related Issues**: Use the parent issue feature to track failures related to specific work items
4. **Monitor Patterns**: Watch for recurring failures that might indicate systemic issues

## Related Documentation

- [GitHub Actions Workflow](.github/workflows/build-esp32.yml)
- [PlatformIO Build Configuration](../platformio.ini)
- [Project README](../README.md)
