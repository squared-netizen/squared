# assets

Files here are packaged into the APK by `make apk` and reach a running app
through Android's AAssetManager. Nothing outside this directory does.

Load them through SFML:

```cpp
sf::Texture texture;
texture.loadFromFile("picture.png");

sf::Font font;
font.openFromFile("font.ttf");

sf::Music music;
music.openFromFile("tune.ogg");
```

Open by **bare name, relative to this directory**. Subdirectories survive:

```
sq_android/assets/font.ttf        ->  "font.ttf"
sq_android/assets/ui/panel.png    ->  "ui/panel.png"
```

The whole tree is copied into the APK under `assets/` and read from there in
place — nothing is unpacked to storage.

`make android-status` reports how many files were found here. An empty
directory packages nothing — aapt2 rejects an empty asset directory, so the
`-A` flag is passed only when there is something to pass it for.

This README is packaged too. Delete it once you have real assets, or leave it;
a few hundred bytes in an APK is not worth a decision.
