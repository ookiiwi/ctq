## CTQ (Compressed TEI Query)

Encodes [TEI dictionaries](https://tei-c.org/release/doc/tei-p5-doc/en/html/DI.html) into indexed compressed file format. It can then quickly lookup and decode entries.

## Build

```console
$ mkdir build
$ cd build
$ cmake .. <options>
$ cmake --build .
```

## Testing

```
$ cd build
$ cmake .. -DBUILD_TESTING=ON
$ cmake --build .
$ ./test/ctq_test
```

## CMake project options

* `BUILD_TESTING`    -- When `ON` ctq's test binary will be built. Defaults to `OFF`.
* `BUILD_CTQ_WRITER` -- When `ON` ctq's encoding library will be built. Defaults to `OFF`.
* `BUILD_CTQ_READER` -- When `ON` ctq's decoding library will be built. Defaults to `ON`.
* `BUILD_CTQ_CLI`    -- When `ON` ctq's command line interface bundling writer and reader will be built. Defaults to `OFF`.

## External resources

- [lz4](https://github.com/lz4/lz4) is under the [BSD 2-Clause license and GPLv2 license](https://github.com/lz4/lz4/blob/dev/LICENSE)
- [LibXml2](https://gitlab.gnome.org/GNOME/libxml2) is under the [MIT license](https://gitlab.gnome.org/GNOME/libxml2#license)
- [xcdat](https://github.com/ookiiwi/xcdat) is under the [MIT license](https://github.com/ookiiwi/xcdat/blob/master/LICENSE)
- [Catch2](https://github.com/catchorg/Catch2) is under the [BSL-1.0 license](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt)
