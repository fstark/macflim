# How to release MacFlim?

Just add a tag like:

```
git tag -a v2.0.16 -m "Release v2.0.16"
git push origin v2.0.16
```

This will build everything and create a draft release. Go to [github and publish the release](https://github.com/fstark/macflim/releases) if correct.

# How to "un-release MacFlim"?

Delete the local tag and the global one

``
git tag -d v2.0.16
git push --delete origin v2.0.16
``

Example of what's new section:

## What's New?

Fixed memory allocation of Mini MacFlim for Multi-Finder execution (default of 350K is too low as prefs wants 300K of buffers)
