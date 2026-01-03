# How to release MacFlim?

Just add a tag like:

```
git tag -a v2.0.14 -m "Release v2.0.14"
git push origin v2.0.14
```

This will build everything and create a draft release. Go to [github and publish the release](https://github.com/fstark/macflim/releases) if correct.

# How to "un-release MacFlim"?

Delete the local tag and the global one

``
git tag -d v2.0.14
git push --delete origin v2.0.14
``

Example of what's new section:

## What's New?

This releases fixes a playback bug: flims of the same size as the screen were crashing the machine with MacFlim (the XCMD was ok).

Added ``--anchor-x and --anchor-y`` options to ``flimmaker`` to control the crop window. See README.md for details.
