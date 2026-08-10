# WEB_SRC

`WEB_SRC/` stores the checked-in sources for the embedded Web UI.

Files:
- `page-app.js`: main frontend source for `WebPageAppJs.h`.
- `app.css`: frontend stylesheet for `WebPageAppCss.h`.
- `uplot.min.js`: vendored `uPlot` JavaScript asset for `WebUplotJs.h`.
- `uplot.min.css`: vendored `uPlot` stylesheet asset for `WebUplotCss.h`.
- `generate_embedded_web.py`: reproducible header generator.

Notes:
- `uPlot` version is tracked as `1.6.32` based on the project history and the ignored vendor archive paths in `.gitignore`.
- `OUT/app.css` was found stale relative to `WebPageAppCss.h`; the maintained source in this directory must match the embedded header payload.
- The generator pins gzip `mtime` to `0x6A574F3E` so `uPlot` headers stay byte-identical to the current embedded assets.

Regenerate all embedded Web UI headers from the repository root:

```bash
python WEB_SRC/generate_embedded_web.py all
```

Verify that generated headers are already up to date:

```bash
python WEB_SRC/generate_embedded_web.py all --check
```

Generate one text header explicitly:

```bash
python WEB_SRC/generate_embedded_web.py text --source WEB_SRC/page-app.js --output WebPageAppJs.h --symbol PAGE_APP_JS --source-label WEB_SRC/page-app.js
```

Generate one gzip header explicitly:

```bash
python WEB_SRC/generate_embedded_web.py gzip --source WEB_SRC/uplot.min.js --output WebUplotJs.h --symbol UPLOT_JS_GZ --source-label WEB_SRC/uplot.min.js --gzip-mtime 0x6A574F3E
```
