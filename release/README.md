# How to release MacFlim?

Just add a tag like:

```
git tag -a v2.0.19 -m "Release v2.0.19"
git push origin v2.0.19
```

This will build everything and create a draft release. Go to [github and publish the release](https://github.com/fstark/macflim/releases) if correct.

# How to "un-release MacFlim"?

Delete the local tag and the global one

``
git tag -d v2.0.17
git push --delete origin v2.0.17
``

Example of what's new section:

## What's New?

Added a new dithering option, blue noise. This is a good tradeoff between the regularity of `ordered` dithering and the crispness of `error`. Enable it using `--dither blue` (https://cv.ulichney.com/papers/1988-blue-noise.pdf).

Implemented and documented all the `--pgm-xxx` debug output options:
- `--pgm` now outputs actual encoded frames (what goes into the FLIM file)
- `--pgm-poster` outputs 128×86 poster thumbnails
- `--pgm-diff` outputs difference between encoded and source
- `--pgm-change` outputs difference between consecutive frames
- `--pgm-target` outputs target source images

Deprecated redundant command-line arguments: `--out-pattern` and `--pgm-pattern` (use `--pgm`), `--diff-pattern` (use `--pgm-diff`), `--change-pattern` (use `--pgm-change`), `--target-pattern` (use `--pgm-target`).
