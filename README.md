# libdmg-hfsplus

This is a stripped down version of [libdmg-hfsplus](https://github.com/planetbeing/libdmg-hfsplus), which only contains the `dmg` tool.

Ideally, the minimal c code could be ported to Rust. We could also
reimplement the build ISO functionality and skip the `genisoimage`
step during gitian building.

## Build

`zlib` is required.

Configure, compile and run:
```bash
cmake . -B build
make -C build/dmg -j8
build/dmg/dmg
```

Compare DMG output for a Bitcoin-Core ISO (from `genisoimage`):
```bash
git clean -fxd && \
cmake . -B build && \
make -C build/dmg -j8 && \
build/dmg/dmg Bitcoin-Core.iso osx-unsigned.dmg && \
diffoscope osx-unsigned-orig.dmg osx-unsigned.dmg
```

Running Clang tools:
```bash
clang-format -i dmg/*.c
clang-tidy -p build --checks=modernize dmg/*.c
```

## DMG Notes

http://newosxbook.com/DMG.html

## License

This work is released under the terms of the GNU General Public License,
version 3. The full text of the license can be found in the [LICENSE](LICENSE) file.
