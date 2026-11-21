AETHER MINE LUA GAME STUDIO V2.0 — STARTER PATCH

This starter patch adds a first 2D Canvas game runtime to the existing Lua 5.4 Android project.

Files to replace:
1. app/src/main/cpp/native-lib.cpp
2. app/src/main/java/com/aethermine/lua/MainActivity.java
3. app/src/main/assets/index.html

Important:
- Keep your existing CMakeLists.txt and Lua 5.4.9 source files unchanged.
- Keep the existing Gradle files and GitHub Actions workflow unchanged for this first test.
- Apply these changes only on the v2-game-studio branch, not main.
- The native code expects the existing package name com.aethermine.lua and library name aetherlua.
- The new API supports game.create, game.background, game.rect, game.circle, game.line, game.text and game.key.
- This is the first 2D runtime milestone. Sprite images, asset manager, collision helpers, audio, and 3D are not included yet.
- The game runs Lua update()/draw() callbacks through a JavaScript animation loop. Each frame is a synchronous JNI call; keep update/draw lightweight.
- The existing ordinary Lua runLua() function remains available.

Build and test:
1. Replace all three files on branch v2-game-studio.
2. Run GitHub Actions Build Debug APK.
3. Install the APK.
4. Open the built-in 2D Platformer Demo and test left/right/jump.
