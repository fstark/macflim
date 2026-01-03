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
git tag -d v2.0.15
git push --delete origin v2.0.15
``

Example of what's new section:

## What's New?

Added a Stuff 1.5.1 archive to the release artifcats, containing MacFlim, Mini MacFlim and MacFlim XCMD

Note:
* Mini MacFlim is broken in this release
* The MacFlim XCMD is missing the demo flim
* There are no flims included in the .sit
