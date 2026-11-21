AETHER MINE LUA GAME STUDIO V2.1 — PHASE 2

Files:
1) app/src/main/cpp/native-lib.cpp
2) app/src/main/java/com/aethermine/lua/MainActivity.java

IMPORTANT:
- Make these changes ONLY on branch v2.1-game-studio.
- Keep main and v2-game-studio unchanged.
- This phase adds native input state and Lua API tables:
    input.down(name)
    input.action(id)
    input.axisX(), input.axisY()
    input.pointerX(), input.pointerY(), input.pointerDown()
    ui.panel(id,x,y,w,h,color)
    ui.text(id,text,x,y,size,color)
    ui.button(id,label,x,y,w,h) -> returns true while action id is active
- Existing game.key(name) remains as a compatibility alias for input.down(name).
- Existing game drawing commands remain.

IMPORTANT RUNTIME CONTRACT:
The JavaScript runtime must call:
AetherLua.frameInput(keysCsv, axisX, axisY, pointerX, pointerY, pointerDown, actionsCsv)
where keysCsv and actionsCsv are comma-separated names.
The native engine emits @GAME, @PRINT, and @UI tab-separated records.
The JS renderer must parse @UI records and render them. This native phase alone
does NOT draw the UI commands or create visible touch controls.

Lua sample:
function update()
  if input.down("left") then x = x - 3 end
  x = x + input.axisX() * 3
  if input.pointerDown() then print("pointer", input.pointerX(), input.pointerY()) end
end

function draw()
  game.rect(x, 100, 40, 40, "#ff3344")
  if ui.button("start","START",20,20,120,45) then
    print("START clicked")
  end
end

Before building:
- Confirm Lua headers and existing CMake target are unchanged.
- Do not change package name or library name.
- Build on v2.1-game-studio branch.
