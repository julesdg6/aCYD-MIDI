# AI Costs Update Guide

This guide explains how to update the AI development costs displayed on the documentation landing page with actual billing data.

## Current Display

The main documentation page (`docs/index.html`) includes an "AI-Assisted Development Stats" section that shows:
- Number of merged Copilot PRs
- Development timeline
- Files modified
- Estimated AI development costs

## Updating Cost Estimates with Actual Data

### Step 1: Access GitHub Copilot Usage Data

1. Go to your GitHub organization/account settings
2. Navigate to **Billing & plans** → **Copilot**
3. View usage reports and actual costs incurred

### Step 2: Update the HTML

Edit `docs/index.html` and locate the `.cost-estimate` section (around line 360-390).

Update these values:

```html
<div class="cost-breakdown">
    <div class="cost-item">
        <span>GitHub Copilot Workspace Sessions (61 PRs × ~30 min avg)</span>
        <strong>$120-180</strong>  <!-- Update this with actual cost -->
    </div>
    <div class="cost-item">
        <span>Additional iterations and refinements</span>
        <strong>$40-60</strong>  <!-- Update this with actual cost -->
    </div>
    <div class="cost-item">
        <span>Code reviews and security scans</span>
        <strong>$20-40</strong>  <!-- Update this with actual cost -->
    </div>
    <div class="cost-item" style="border-top: 2px solid rgba(255, 255, 255, 0.3); margin-top: 10px; padding-top: 15px;">
        <span><strong>Total Estimated AI Development Cost</strong></span>
        <strong style="font-size: 1.3em; color: #00d4ff;">$180-280</strong>  <!-- Update total -->
    </div>
</div>
```

### Step 3: Update Statistics (Optional)

You can also update other statistics in the section:

```html
<div class="stats-grid">
    <div class="stat-card">
        <div class="stat-value">61</div>  <!-- Update PR count -->
        <div class="stat-label">Merged Copilot PRs</div>
    </div>
    <div class="stat-card">
        <div class="stat-value">~28</div>  <!-- Update days -->
        <div class="stat-label">Days of Development</div>
    </div>
    <div class="stat-card">
        <div class="stat-value">100+</div>  <!-- Update file count -->
        <div class="stat-label">Files Modified</div>
    </div>
    <div class="stat-card">
        <div class="stat-value">~75%</div>  <!-- Update percentage -->
        <div class="stat-label">AI-Generated Code</div>
    </div>
</div>
```

### Step 4: Update the Disclaimer

If you're using actual billing data, update the disclaimer:

```html
<div class="disclaimer">
    <strong>✅ Note:</strong> These costs are based on actual GitHub Copilot billing data 
    as of [DATE]. Your usage may vary.
</div>
```

## Calculating Actual Costs

### From GitHub Billing Dashboard

1. **Copilot Workspace**: Check usage hours and multiply by the per-hour rate
2. **Code Reviews**: Count automated review sessions
3. **Security Scans**: Count CodeQL runs triggered by Copilot

### Cost Breakdown Example

If you have access to detailed billing:

```
Copilot Workspace Sessions:
- 61 PRs
- Average 25 minutes per PR
- Total: 25.4 hours
- Rate: $0.08/minute or $4.80/hour
- Cost: 25.4 × $4.80 = $122

Additional Iterations:
- Re-runs and refinements: $45

Code Reviews & Security:
- Automated reviews: $25

Total: $192
```

## Automating Updates

For more frequent updates, you could:

1. **Manual Script**: Create a script that queries GitHub API for PR counts
2. **GitHub Actions**: Set up a workflow to update the stats monthly
3. **Dynamic Content**: Use JavaScript to fetch stats from a JSON file

Example JSON approach:

```json
{
  "stats": {
    "mergedPRs": 61,
    "developmentDays": 28,
    "filesModified": "100+",
    "aiPercentage": "~75%"
  },
  "costs": {
    "workspace": 122,
    "iterations": 45,
    "reviews": 25,
    "total": 192
  },
  "lastUpdated": "2026-02-07"
}
```

## Questions?

If you need help updating the costs or have questions about the calculations, please:
1. Check GitHub's Copilot billing documentation
2. Contact GitHub support for detailed usage reports
3. Open an issue in this repository for technical help with the webpage

## See Also

- [GitHub Copilot Billing Documentation](https://docs.github.com/en/billing/managing-billing-for-github-copilot)
- [GitHub Copilot Usage Reports](https://docs.github.com/en/copilot/managing-copilot/managing-copilot-as-an-individual-subscriber/managing-your-copilot-subscription/viewing-your-github-copilot-usage)
