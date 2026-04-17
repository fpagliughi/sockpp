# Manually Triggering the Docs Workflow

The `docs.yml` workflow deploys the combined mdBook + Doxygen site to GitHub
Pages. It runs automatically on every push to `master` or `gh-pages`, but can
also be triggered manually in three ways.

## GitHub Web UI

1. Go to the repository on GitHub.
2. Click the **Actions** tab.
3. Select **Deploy documentation to Pages** from the workflow list on the left.
4. Click **Run workflow**, choose the branch, then click **Run workflow**.

## GitHub CLI

```bash
gh workflow run docs.yml
```

To target a specific branch:

```bash
gh workflow run docs.yml --ref master
```

## GitHub REST API

```bash
curl -X POST \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  https://api.github.com/repos/fpagliughi/sockpp/actions/workflows/docs.yml/dispatches \
  -d '{"ref":"master"}'
```
