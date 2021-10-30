# libdmg-hfsplus

This is a stripped down version of [libdmg-hfsplus](https://github.com/planetbeing/libdmg-hfsplus), which only contains the `dmg` tool.

## Build

`zlib` is required. On Ubuntu this means `sudo apt-get install zlib1g zlib1g-dev`

Configure, compile and run:
```bash
cmake . -B build
make -C build/dmg -j8
build/dmg/dmg
```


## DMG Notes

http://newosxbook.com/DMG.html
https://developer.apple.com/library/archive/technotes/tn2166/_index.html

## License

This work is released under the terms of the GNU General Public License,
version 3. The full text of the license can be found in the [LICENSE](LICENSE) file.
