# libdmg-hfsplus

This is a stripped down version of [libdmg-hfsplus](https://github.com/planetbeing/libdmg-hfsplus), which only contains the `dmg` tool.

## Build

`zlib` is required.

Configure, compile and run using:
```bash
cmake . -B build
make -C build/dmg -j8
build/dmg/dmg
```

## License

This work is released under the terms of the GNU General Public License,
version 3. The full text of the license can be found in the [LICENSE](LICENSE) file.
