# How to release MacFlim?

Just add a tag like:

```
git tag -a v2.0.12 -m "- Added version embedding in binaries/about panels/disk names/flims (use --version in flimmaker to display version)
- The macsrc/ directory is now always in sync with the content of the disk image and contains fixed filenames
- Release is now fully automated and produce both windows and vintage Mac binaries (so hopefully easier to update)"
git push origin v2.0.12
```

This will build everything and create a draft release. Go to [github and publish the release](https://github.com/fstark/macflim/releases) if correct.

# How to "un-release MacFlim"?

Delete the local tag and the global one

``
git tag -d v2.0.12
git push --delete origin v2.0.12
``

Example of what's new section:

## What's new?

This is a consolidation release. Not all the code was in sync with the repository content, and the code of the XCMD was diverging from the MacFlim player codebase.

Everything is now completely automated, so hopefully I'll be able to do more regular updates.

Also, binaries now know what version they are. Use ``flimmaker --version`` to check version, or the about panel of MacFlim.