AETHER MINE LUA GAME STUDIO V2.1 — PHASE 1

Included:
- index.html: V2.1 Home, Editor, Save Done, Apply and Game Start foundation.

IMPORTANT:
1. Keep the existing v2-game-studio branch unchanged.
2. Create a new branch named v2.1-game-studio from v2-game-studio.
3. In that new branch, replace only:
   app/src/main/assets/index.html
4. Do not replace MainActivity.java, native-lib.cpp, CMakeLists.txt or Lua source files yet.
5. Commit the change, then build the APK from v2.1-game-studio.

Current compatibility:
- This page uses the existing V2.0 Android bridge methods:
  AetherLua.runGame(), AetherLua.frame(), AetherLua.stopGame(),
  AetherLua.nativeEngineStatus().
- The UI foundation is prepared. The selected touch/joystick profiles are saved as settings,
  but actual virtual touch controls and the expanded Lua/App UI APIs require the next native
  engine phase. Do not assume those controls are already active.
