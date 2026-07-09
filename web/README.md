# Onager Playground

A static web app that runs Onager graph analytics in the browser using
[DuckDB-WASM](https://duckdb.org/docs/stable/clients/wasm/overview). Everything runs client
side: DuckDB-WASM is loaded from jsDelivr, and the Onager WASM extension is served same-origin
from the `extensions/` directory that CI places next to this app.

It is deployed as part of the documentation site at
`https://cogitatortech.github.io/onager/playground/`.

## Contents

- `index.html`: page layout (guided demos, SQL editor, results table).
- `app.js`: DuckDB-WASM initialization, Onager extension loading, and query execution. No build
  step and no npm dependencies; DuckDB-WASM is imported as an ES module from a CDN.
- `style.css`: styling that matches the docs palette.
- `logo.svg`: copied from `docs/assets/logo.svg`.

## How It Is Built and Deployed

- `.github/workflows/web.yml` builds the `wasm_eh` and `wasm_mvp` Onager extension against
  DuckDB `v1.5.4`, assembles a `playground/` tree (a copy of this directory plus the extension
  under `extensions/`), and uploads it as the `playground` workflow artifact.
- `.github/workflows/docs.yml` downloads that artifact into `./site/playground/` before the
  single GitHub Pages deployment, so the app is served as a subpath of the docs site.

The DuckDB-WASM version pinned in `app.js` is chosen to match DuckDB `v1.5.4` so the extension
ABI lines up. Only the non-threaded bundles (`wasm_eh`, `wasm_mvp`) are used, because GitHub
Pages cannot send the cross-origin isolation headers the threaded bundle requires.

## Running Locally

The app needs the Onager extension to be reachable. The extension is not built by a local
`make` target, so the simplest local check points the app at the already-deployed extension
directory (GitHub Pages serves it with `Access-Control-Allow-Origin: *`):

```bash
python3 -m http.server --directory web 8000
```

Then open `http://localhost:8000/`. To load the extension while developing locally, temporarily
change the `custom_extension_repository` value in `app.js` to the deployed URL
(`https://cogitatortech.github.io/onager/playground/extensions`). When served from the deployed
site the default same-origin path works without changes.
