# CI Failure Reporting Implementation Summary

## Overview

This document summarizes the implementation of the automated CI failure reporting system for the aCYD-MIDI project.

## What Was Implemented

### 1. Automated Failure Detection
- Added a `report_failure` job to `.github/workflows/build-esp32.yml`
- Job runs automatically when the `build` job fails (using `if: failure()`)
- Executes after build completion, does not block the main workflow

### 2. Full Log Capture
The system fetches complete logs from failed build jobs:
- Uses GitHub REST API to list all jobs in the workflow run
- Identifies jobs with `conclusion === 'failure'`
- Downloads full logs for each failed job via `downloadJobLogsForWorkflowRun`
- Includes up to 30KB of logs per job (truncates if longer with a note)
- Falls back to providing links if API calls fail

### 3. Issue Creation
Automatically creates a GitHub issue with:
- **Title Format**: `CI Build Failure: <Workflow Name> (Run #<Run Number>)`
- **Labels**: `ci-failure`, `automated`
- **Content Includes**:
  - Workflow metadata (name, run ID, actor, event, branch, commit)
  - Failure summary with context
  - Full job logs from all failed jobs
  - Direct links to workflow run, artifacts, and repository
  - Step-by-step debugging instructions

### 4. Parent Issue Linking (Optional)
When a parent issue number is provided:
- Posts a comment on the parent issue
- Comment includes link to the newly created failure issue
- Creates a clear parent→child relationship for tracking

### 5. Manual Trigger Support
Added `workflow_dispatch` trigger:
- Allows manual workflow runs from GitHub Actions UI
- Includes `parent_issue` input parameter
- Enables linking failure issues to specific parent issues

### 6. Permissions Configuration
- Added `permissions: issues: write` at workflow level
- Allows the workflow to create issues and post comments
- Removed redundant permission from `release` job

## Files Modified

1. **`.github/workflows/build-esp32.yml`** (Main implementation)
   - Added `workflow_dispatch` trigger with `parent_issue` input
   - Added `permissions` section
   - Added `report_failure` job with 3 steps:
     - Download build logs (artifacts)
     - Create log summary
     - Fetch job logs and create issue

2. **`docs/CI_FAILURE_REPORTING.md`** (Documentation)
   - Comprehensive user guide
   - Usage examples
   - Troubleshooting section
   - Best practices

3. **`README.md`** (Reference)
   - Added link to CI_FAILURE_REPORTING.md in Documentation section

## How to Use

### Automatic Mode (Default)
No action needed. When builds fail on:
- Push to `main` branch
- Pull requests
- Tagged releases

The system automatically creates failure issues.

### Manual Mode with Parent Issue
1. Go to GitHub Actions → "Build ESP32 firmware (PlatformIO)"
2. Click "Run workflow"
3. Enter parent issue number (e.g., `42`)
4. Click "Run workflow"
5. If build fails, a failure issue is created and linked to issue #42

## Testing the Feature

### Option 1: Wait for Natural Failure
Simply wait for a build to fail naturally, and the system will create an issue.

### Option 2: Intentional Failure (Not Recommended)
You could temporarily introduce a build error to test the system, but this is not recommended as it pollutes the repository history.

### Option 3: Review in Pull Request
When this PR is merged, the next natural build failure will demonstrate the feature.

## What Gets Reported

### Example Issue Structure

```markdown
## 🔴 Automated CI Build Failure Report

**Workflow:** Build ESP32 firmware (PlatformIO)
**Run ID:** #1234567890
**Run Number:** #42
**Triggered by:** @username
**Event:** push
**Branch/Ref:** `refs/heads/main`
**Commit:** `abc1234`

---

### 📋 Failure Summary

```
====================================
CI BUILD FAILURE REPORT
====================================

Workflow: Build ESP32 firmware (PlatformIO)
Run ID: 1234567890
...
====================================
```

---

## Failed Job Details

### Job: build (esp32-2432S028Rv2)
- **Status:** completed
- **Conclusion:** failure
- **Started:** 2024-01-01T12:00:00Z
- **Completed:** 2024-01-01T12:05:00Z
- **Job URL:** https://github.com/...

#### Full Job Logs:
```
[Full compilation output, error messages, etc.]
```

---

### 🔗 Useful Links
[Links to workflow run, artifacts, repository]

---

### 🛠️ How to Debug
[Step-by-step debugging instructions]
```

## Implementation Details

### API Calls Used
- `github.rest.actions.listJobsForWorkflowRun` - Get all jobs
- `github.rest.actions.downloadJobLogsForWorkflowRun` - Get logs for specific job
- `github.rest.issues.create` - Create failure issue
- `github.rest.issues.createComment` - Link to parent issue

### Error Handling
- All API calls use `continue-on-error: true` or try-catch
- Graceful degradation if API calls fail
- Always provides workflow run URL as fallback

### Log Size Management
- Limits each job's logs to 30KB to avoid GitHub issue size limits
- Provides truncation notice with link to full logs
- Multiple failed jobs can each contribute up to 30KB

## Security Considerations

### CodeQL Analysis
✅ **Passed**: No security issues detected

### Token Usage
- Uses built-in `GITHUB_TOKEN` (no custom PAT needed)
- Token has minimal required permissions (`issues: write`)
- Token is automatically scoped to the repository

### Input Validation
- Parent issue number is parsed as integer
- Empty/undefined values are handled gracefully
- No risk of injection attacks (all values templated safely)

## Benefits

1. **Faster Issue Tracking**: Build failures are immediately documented
2. **Complete Context**: All relevant information in one place
3. **Easy Debugging**: Full logs directly in the issue
4. **Parent-Child Relationships**: Link failures to ongoing work
5. **Automation**: Zero manual effort for failure tracking
6. **Transparency**: Clear audit trail of build problems

## Future Enhancements

Potential improvements (not implemented):
- Attach full log files as artifacts to issues
- Parse and extract specific error messages
- Auto-assign issues based on file paths
- Trend analysis (repeated failures)
- Integration with Slack/Discord notifications
- Auto-close when fixed

## Verification Checklist

- [x] YAML syntax validated
- [x] Workflow structure verified
- [x] Code review completed and issues addressed
- [x] Security scan (CodeQL) passed
- [x] Documentation created
- [x] README updated
- [x] Permissions configured correctly
- [x] Error handling implemented
- [x] Log truncation handled

## Summary

The CI failure reporting system is fully implemented and ready to use. It will automatically create detailed GitHub issues with full logs whenever builds fail, and can optionally link these failures to parent issues when manually triggered. The implementation is secure, well-documented, and requires no ongoing maintenance.
