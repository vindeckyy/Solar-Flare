# GitHub Pages deployment (reference)

> [!NOTE]
> This directory is an **inherited Jekyll template**. It is **not** what publishes
> the live SolarFlare website. Do not edit these files for user-facing content.

---

## What actually deploys

| Component | Path | Role |
|---|---|---|
| **Live site** | https://vindeckyy.github.io/Solar-Flare/ | Marketing + docs portal |
| **Source** | `website/` | Next.js static export (App Router) |
| **Workflow** | `.github/workflows/update-pages.yml` | Build and push `website/out/` to `gh-pages` branch |
| **This directory** | `gh-pages-template/` | Legacy template - unused |

---

## Maintainer publish flow

1. **Edit content** in `website/`:
   - Marketing: `website/components/solarflare/`
   - Documentation articles: `website/lib/docs-data.ts`
   - Routes: `website/app/`

2. **Build locally** (optional smoke test):

   ```bash
   cd website
   npm ci
   npm run build
   # Output: website/out/
   ```

3. **Merge to `master`** - the `update-pages.yml` workflow runs on the
   configured trigger (typically push to `master`).

4. **Verify deployment**:
   - GitHub → Actions → `update-pages` workflow success
   - Browse https://vindeckyy.github.io/Solar-Flare/
   - Spot-check docs slugs listed in `docs-data.ts`

---

## Sync with repository Markdown

Website docs are authored in TypeScript (`docs-data.ts`), not by copying
`docs/*.md` at build time. When expanding repository documentation:

1. Update the canonical Markdown in `docs/`
2. Mirror factual changes in `website/lib/docs-data.ts`
3. Rebuild `website/` and verify rendered pages

The Autoprompt doc mission treats both surfaces as first-class. Drift between
Markdown and `docs-data.ts` is a documentation bug.

---

## gh-pages branch vs this template

GitHub Pages serves the **`gh-pages` branch** (or `out/` artifact from Actions),
not files from `gh-pages-template/`. The template remains in the tree for
historical reference and upstream merge compatibility.

**Do not** manually copy `gh-pages-template/` to `gh-pages` unless you are
deliberately reverting to the legacy Jekyll site (not supported for SolarFlare).

---

## Troubleshooting deployment

| Symptom | Check |
|---|---|
| Site shows old content | Actions log; cache; confirm `out/` uploaded |
| Docs 404 | Slug in `docs-data.ts` must match `app/docs/[slug]/page.tsx` |
| Build fails in CI | Node version in workflow; `npm ci` lockfile |
| Images broken | Paths under `website/public/` |

See [Maintainers - release](../docs/maintainers/release.md) for version badges
that must match README and `docs-data.ts` after releases.

---

## See also

- [website/README.md](../website/README.md) if present
- [Maintainers README](../docs/maintainers/README.md)
- [Guides - ecosystem](../docs/guides.md)
