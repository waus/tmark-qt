# Tmark for Qt

A C++20 parser and serializer for tmark, plus a Qt 6 QML
module for displaying documents. The QML module provides 
`TmarkView`, themes, and replaceable block renderers.

## Requirements

- CMake 3.21 or newer and a C++20 compiler.
- For the QML module: Qt 6.5 or newer with Core, Gui, Network, Qml, Quick,
  Quick Controls, Quick Effects, and Qt Lottie. Desktop builds also need Qt Multimedia
  and FFmpeg development libraries (`libavformat`, `libavcodec`, `libswscale`).
- For WebP images: the Qt Image Formats WebP plugin.

If Qt is unavailable, CMake builds only the codec.

## Native desktop build

From this repository's root, point CMake at a Qt kit for your host platform.
For the macOS kit used in this workspace:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$HOME/sdk/Qt/6.11.3/macos" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

On Linux or Windows, replace `CMAKE_PREFIX_PATH` with the path to the native
Qt kit for that platform. CMake also needs `pkg-config` to find the FFmpeg
development libraries listed above. The example application is optional.

To install the libraries, run:

```sh
cmake --install build --prefix /path/to/tmark-install
```

In a consuming CMake project, point `CMAKE_PREFIX_PATH` at the install prefix
and link the targets you need:

```cmake
find_package(Tmark CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Tmark::Qml)   # Includes the codec
# Or, for a C++ application without Qt:
# target_link_libraries(my_app PRIVATE Tmark::Codec)
```

Make `<install-prefix>/lib/qml` available to the QML engine through
`QML_IMPORT_PATH` or `QQmlEngine::addImportPath()`; use the platform's CMake
install library directory if it differs from `lib`.

## Display a document

```qml
import QtQuick
import io.github.waus.tmark

TmarkView {
    anchors.fill: parent
    text: documentText
    softParser: true
    theme: TmarkViewTheme {
        font.pixelSize: 16
    }
}
```

`text` accepts a complete tmark document. `softParser` defaults to `false`;
enable it to preserve unknown tags. The read-only `error` property reports a
parse error, and `parsedDocument` exposes the parsed tree. `TmarkViewTheme`
uses the system palette by default and lets you override fonts and colors.

To replace a block renderer, map its tmark tag to a QML `Component`:

```qml
TmarkView {
    text: documentText
    customWidgets: ({ "hr": customDivider })

    Component {
        id: customDivider
        Rectangle {
            property var node
            implicitHeight: 2
            color: "steelblue"
        }
    }
}
```

A custom component can declare `node` and `theme` properties to receive the
parsed block and current theme. See [the example](example/Main.qml) for a
document picker that loads examples from the network.

## Use the C++ codec

```cpp
#include <tmark/codec.hpp>

const auto document = tmark::parse(
    "{document;#url{https://example.com}#title{Hello}{p;Hello, {b;world}!}}");
const std::string wire = tmark::serialize(document);
```

`parse` and `serialize` throw `tmark::Error` for invalid input. Both accept an
optional `bool soft` argument to preserve unknown tags. The public model is
`tmark::Node`, with fields and content represented by `tmark::Field` and
`tmark::Part`.

## Media and platform support

The default `TmarkMediaLoader` fetches HTTP and HTTPS URLs. Supply a subclass
through `TmarkView.mediaLoader` to handle other URL schemes. The library checks
downloaded content instead of trusting a URL extension or Content-Type, and
limits each media download to 128 MiB.

| Platform | Images | Audio and video |
| --- | --- | --- |
| Desktop | JPEG, PNG, WebP, AVIF, Lottie JSON | MP3, FLAC, Ogg/Opus, MP4/AAC-LC audio; video through Qt Multimedia |
| WebAssembly | JPEG, PNG, WebP, Lottie JSON | Video preview images only; playback unavailable with the current Qt WebAssembly kit |

The viewer supports headings, formatted text, links, lists, tasks, tables,
quotes, details, images, galleries, spoilers, captions, audio, and video.
Math blocks are shown as source text; maps are not rendered. Media
on the web is subject to browser CORS rules.

## Example application

To build and run `tmark_example`, configure with `-DTMARK_BUILD_EXAMPLE=ON`,
then run `cmake --build build --parallel` and `./build/tmark_example`.
It loads the example documents from `https://tmark.waus.app/examples/`;
no documents or media are bundled with it.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

The media validation test downloads sample files from `tmark.waus.app` and
requires network access. Codec and QML tests use fixtures included in this
repository and run offline.
