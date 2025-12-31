# How to release MacFlim?

Just add a tag like:

```
git tag -a v2.0.12 -m "- Added version embedding in binaries/about panels/disk names
- Made macsrc always in sync with the content of the disk image
- Release automation"
git push origin v2.0.12
```

# How to "un-release MacFlim"?

Delete the local tag and the global one

``
git tag -d v2.0.12
git push --delete origin v2.0.12
``
