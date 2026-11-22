#include <jni.h>
#include <string>
#include <sstream>
#include <mutex>
#include <cmath>
#include <unordered_set>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static std::mutex luaMutex;
static lua_State* gameState = nullptr;
static std::ostringstream gameOutput;
static std::ostringstream gameCommands;
static const char* GAME_PREFIX = "@GAME\t";
static const char* PRINT_PREFIX = "@PRINT\t";
static const char* UI_PREFIX = "@UI\t";

static float gAxisX = 0.0f, gAxisY = 0.0f;
static float pointerX = 0.0f, pointerY = 0.0f;
static bool pointerIsDown = false;
static bool pointerPressed = false, pointerReleased = false;
static std::unordered_set<std::string> previousKeys, previousActions;

static std::string escapeField(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '\t') out += "\\t";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

static void emitGame(const std::string& command) {
    gameCommands << GAME_PREFIX << command << "\n";
}
static void emitUI(const std::string& command) {
    gameCommands << UI_PREFIX << command << "\n";
}

static int capturePrint(lua_State* L) {
    int n = lua_gettop(L);
    std::ostringstream line;
    for (int i = 1; i <= n; ++i) {
        if (i > 1) line << "\t";
        size_t len = 0;
        const char* value = luaL_tolstring(L, i, &len);
        if (value) line.write(value, static_cast<std::streamsize>(len));
        lua_pop(L, 1);
    }
    gameOutput << PRINT_PREFIX << escapeField(line.str()) << "\n";
    return 0;
}

static void executionLimit(lua_State* L, lua_Debug*) {
    int* remaining = static_cast<int*>(lua_getextraspace(L));
    *remaining -= 1000;
    if (*remaining <= 0) luaL_error(L, "Execution limit exceeded.");
}
static void setInstructionBudget(lua_State* L) {
    *static_cast<int*>(lua_getextraspace(L)) = 120000;
    lua_sethook(L, executionLimit, LUA_MASKCOUNT, 1000);
}

static int gameCreate(lua_State* L) {
    int w = (int)luaL_checkinteger(L, 1), h = (int)luaL_checkinteger(L, 2);
    if (w < 1 || w > 4096 || h < 1 || h > 4096)
        return luaL_error(L, "game.create width/height must be 1..4096");
    emitGame("create\t" + std::to_string(w) + "\t" + std::to_string(h));
    return 0;
}
static int gameBackground(lua_State* L) {
    emitGame("background\t" + escapeField(luaL_checkstring(L, 1)));
    return 0;
}
static int gameRect(lua_State* L) {
    double x=luaL_checknumber(L,1), y=luaL_checknumber(L,2);
    double w=luaL_checknumber(L,3), h=luaL_checknumber(L,4);
    const char* c=luaL_checkstring(L,5);
    emitGame("rect\t"+std::to_string(x)+"\t"+std::to_string(y)+"\t"+
             std::to_string(w)+"\t"+std::to_string(h)+"\t"+escapeField(c));
    return 0;
}
static int gameCircle(lua_State* L) {
    double x=luaL_checknumber(L,1), y=luaL_checknumber(L,2);
    double r=luaL_checknumber(L,3); const char* c=luaL_checkstring(L,4);
    emitGame("circle\t"+std::to_string(x)+"\t"+std::to_string(y)+"\t"+
             std::to_string(r)+"\t"+escapeField(c));
    return 0;
}
static int gameLine(lua_State* L) {
    double x1=luaL_checknumber(L,1), y1=luaL_checknumber(L,2);
    double x2=luaL_checknumber(L,3), y2=luaL_checknumber(L,4);
    const char* c=luaL_checkstring(L,5);
    emitGame("line\t"+std::to_string(x1)+"\t"+std::to_string(y1)+"\t"+
             std::to_string(x2)+"\t"+std::to_string(y2)+"\t"+escapeField(c));
    return 0;
}
static int gameText(lua_State* L) {
    const char* t=luaL_checkstring(L,1);
    double x=luaL_checknumber(L,2), y=luaL_checknumber(L,3);
    int size=(int)luaL_optinteger(L,4,24);
    const char* c=luaL_optstring(L,5,"#ffffff");
    emitGame("text\t"+escapeField(t)+"\t"+std::to_string(x)+"\t"+
             std::to_string(y)+"\t"+std::to_string(size)+"\t"+escapeField(c));
    return 0;
}


static int gameTriangle(lua_State* L) {
    double x1=luaL_checknumber(L,1), y1=luaL_checknumber(L,2);
    double x2=luaL_checknumber(L,3), y2=luaL_checknumber(L,4);
    double x3=luaL_checknumber(L,5), y3=luaL_checknumber(L,6);
    const char* color=luaL_optstring(L,7,"#ffffff");
    emitGame("triangle\t"+std::to_string(x1)+"\t"+std::to_string(y1)+"\t"+
             std::to_string(x2)+"\t"+std::to_string(y2)+"\t"+
             std::to_string(x3)+"\t"+std::to_string(y3)+"\t"+escapeField(color));
    return 0;
}

static int gameImage(lua_State* L) {
    const char* src=luaL_checkstring(L,1);
    double x=luaL_checknumber(L,2), y=luaL_checknumber(L,3);
    double w=luaL_checknumber(L,4), h=luaL_checknumber(L,5);
    if (!src[0]) return luaL_error(L, "game.image source cannot be empty");
    if (!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(h))
        return luaL_error(L, "game.image coordinates must be finite numbers");
    emitGame("image\t"+escapeField(src)+"\t"+std::to_string(x)+"\t"+
             std::to_string(y)+"\t"+std::to_string(w)+"\t"+std::to_string(h));
    return 0;
}

static int gameSprite(lua_State* L) {
    const char* src=luaL_checkstring(L,1);
    double x=luaL_checknumber(L,2), y=luaL_checknumber(L,3);
    double w=luaL_checknumber(L,4), h=luaL_checknumber(L,5);
    double sx=luaL_optnumber(L,6,0), sy=luaL_optnumber(L,7,0);
    double sw=luaL_optnumber(L,8,0), sh=luaL_optnumber(L,9,0);
    if (!src[0]) return luaL_error(L, "game.sprite source cannot be empty");
    if (!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(h)||
        !std::isfinite(sx)||!std::isfinite(sy)||!std::isfinite(sw)||!std::isfinite(sh))
        return luaL_error(L, "game.sprite coordinates must be finite numbers");
    emitGame("sprite\t"+escapeField(src)+"\t"+std::to_string(x)+"\t"+
             std::to_string(y)+"\t"+std::to_string(w)+"\t"+std::to_string(h)+"\t"+
             std::to_string(sx)+"\t"+std::to_string(sy)+"\t"+
             std::to_string(sw)+"\t"+std::to_string(sh));
    return 0;
}

static int gameCamera(lua_State* L) {
    double x=luaL_checknumber(L,1), y=luaL_checknumber(L,2);
    double zoom=luaL_optnumber(L,3,1.0);
    if (!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(zoom))
        return luaL_error(L, "game.camera values must be finite numbers");
    if (zoom<0.1) zoom=0.1;
    if (zoom>8.0) zoom=8.0;
    emitGame("camera\t"+std::to_string(x)+"\t"+std::to_string(y)+"\t"+
             std::to_string(zoom));
    return 0;
}

static bool tableFlag(lua_State* L, const char* tableName, const char* key) {
    lua_getglobal(L, tableName);
    if (!lua_istable(L, -1)) { lua_pop(L,1); return false; }
    lua_getfield(L, -1, key);
    bool value = lua_toboolean(L, -1) != 0;
    lua_pop(L,2);
    return value;
}
static int inputDown(lua_State* L) {
    const char* key = luaL_checkstring(L,1);
    lua_pushboolean(L, tableFlag(L, "__aether_keys", key));
    return 1;
}
static int inputAction(lua_State* L) {
    const char* id = luaL_checkstring(L,1);
    lua_pushboolean(L, tableFlag(L, "__aether_actions", id));
    return 1;
}
static int inputPressed(lua_State* L) {
    const char* key = luaL_checkstring(L,1);
    lua_pushboolean(L, tableFlag(L, "__aether_pressed", key));
    return 1;
}
static int inputReleased(lua_State* L) {
    const char* key = luaL_checkstring(L,1);
    lua_pushboolean(L, tableFlag(L, "__aether_released", key));
    return 1;
}
static int pushPointerInfo(lua_State* L) {
    lua_createtable(L,0,5);
    lua_pushnumber(L,pointerX); lua_setfield(L,-2,"x");
    lua_pushnumber(L,pointerY); lua_setfield(L,-2,"y");
    lua_pushboolean(L,pointerIsDown); lua_setfield(L,-2,"down");
    lua_pushboolean(L,pointerIsDown); lua_setfield(L,-2,"active");
    lua_pushboolean(L,pointerPressed); lua_setfield(L,-2,"pressed");
    lua_pushboolean(L,pointerReleased); lua_setfield(L,-2,"released");
    return 1;
}
static int inputPointer(lua_State* L) { return pushPointerInfo(L); }
static int inputTouch(lua_State* L) { return pushPointerInfo(L); }
static int inputAxisX(lua_State* L) { lua_pushnumber(L,gAxisX); return 1; }
static int inputAxisY(lua_State* L) { lua_pushnumber(L,gAxisY); return 1; }
static int inputPointerX(lua_State* L) { lua_pushnumber(L,pointerX); return 1; }
static int inputPointerY(lua_State* L) { lua_pushnumber(L,pointerY); return 1; }
static int inputPointerDown(lua_State* L) { lua_pushboolean(L,pointerIsDown); return 1; }

static int uiPanel(lua_State* L) {
    const char* id=luaL_checkstring(L,1);
    double x=luaL_checknumber(L,2), y=luaL_checknumber(L,3);
    double w=luaL_checknumber(L,4), h=luaL_checknumber(L,5);
    const char* color=luaL_optstring(L,6,"#202838");
    emitUI("panel\t"+escapeField(id)+"\t"+std::to_string(x)+"\t"+
           std::to_string(y)+"\t"+std::to_string(w)+"\t"+std::to_string(h)+
           "\t"+escapeField(color));
    return 0;
}
static int uiText(lua_State* L) {
    const char* id=luaL_checkstring(L,1);
    const char* value=luaL_checkstring(L,2);
    double x=luaL_checknumber(L,3), y=luaL_checknumber(L,4);
    int size=(int)luaL_optinteger(L,5,20);
    const char* color=luaL_optstring(L,6,"#ffffff");
    emitUI("text\t"+escapeField(id)+"\t"+escapeField(value)+"\t"+
           std::to_string(x)+"\t"+std::to_string(y)+"\t"+
           std::to_string(size)+"\t"+escapeField(color));
    return 0;
}
static int uiButton(lua_State* L) {
    const char* id=luaL_checkstring(L,1);
    const char* label=luaL_checkstring(L,2);
    double x=luaL_checknumber(L,3), y=luaL_checknumber(L,4);
    double w=luaL_checknumber(L,5), h=luaL_checknumber(L,6);
    emitUI("button\t"+escapeField(id)+"\t"+escapeField(label)+"\t"+
           std::to_string(x)+"\t"+std::to_string(y)+"\t"+
           std::to_string(w)+"\t"+std::to_string(h));
    lua_pushboolean(L, tableFlag(L, "__aether_actions", id));
    return 1;
}

static void registerGameApi(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L,gameCreate);     lua_setfield(L,-2,"create");
    lua_pushcfunction(L,gameBackground); lua_setfield(L,-2,"background");
    lua_pushcfunction(L,gameRect);       lua_setfield(L,-2,"rect");
    lua_pushcfunction(L,gameCircle);     lua_setfield(L,-2,"circle");
    lua_pushcfunction(L,gameLine);       lua_setfield(L,-2,"line");
    lua_pushcfunction(L,gameText);       lua_setfield(L,-2,"text");
    lua_pushcfunction(L,gameTriangle);    lua_setfield(L,-2,"triangle");
    lua_pushcfunction(L,gameImage);       lua_setfield(L,-2,"image");
    lua_pushcfunction(L,gameSprite);      lua_setfield(L,-2,"sprite");
    lua_pushcfunction(L,gameCamera);      lua_setfield(L,-2,"camera");
    lua_pushcfunction(L,inputDown);      lua_setfield(L,-2,"key");
    lua_setglobal(L,"game");

    lua_newtable(L);
    lua_pushcfunction(L,inputDown);        lua_setfield(L,-2,"down");
    lua_pushcfunction(L,inputAction);      lua_setfield(L,-2,"action");
    lua_pushcfunction(L,inputPressed);     lua_setfield(L,-2,"pressed");
    lua_pushcfunction(L,inputReleased);    lua_setfield(L,-2,"released");
    lua_pushcfunction(L,inputTouch);       lua_setfield(L,-2,"touch");
    lua_pushcfunction(L,inputPointer);     lua_setfield(L,-2,"pointer");
    lua_pushcfunction(L,inputAxisX);       lua_setfield(L,-2,"axisX");
    lua_pushcfunction(L,inputAxisY);       lua_setfield(L,-2,"axisY");
    lua_pushcfunction(L,inputPointerX);    lua_setfield(L,-2,"pointerX");
    lua_pushcfunction(L,inputPointerY);    lua_setfield(L,-2,"pointerY");
    lua_pushcfunction(L,inputPointerDown); lua_setfield(L,-2,"pointerDown");
    lua_setglobal(L,"input");

    lua_newtable(L);
    lua_pushcfunction(L,uiPanel);  lua_setfield(L,-2,"panel");
    lua_pushcfunction(L,uiText);   lua_setfield(L,-2,"text");
    lua_pushcfunction(L,uiButton); lua_setfield(L,-2,"button");
    lua_setglobal(L,"ui");
}

static void restrictLibraries(lua_State* L) {
    luaL_openlibs(L);
    const char* restricted[]={"io","os","debug","package","require","dofile","loadfile"};
    for (const char* name:restricted) { lua_pushnil(L); lua_setglobal(L,name); }
    lua_pushcfunction(L,capturePrint); lua_setglobal(L,"print");
    registerGameApi(L);
}

static void setCsvTable(lua_State* L, const char* globalName, const std::string& csv) {
    lua_newtable(L);
    size_t start=0;
    while (start < csv.size()) {
        size_t end=csv.find(',',start);
        if (end==std::string::npos) end=csv.size();
        std::string key=csv.substr(start,end-start);
        if (!key.empty()) { lua_pushboolean(L,1); lua_setfield(L,-2,key.c_str()); }
        start=end+1;
    }
    lua_setglobal(L,globalName);
}

static bool callLuaFunction(lua_State* L, const char* name) {
    lua_getglobal(L,name);
    if (!lua_isfunction(L,-1)) { lua_pop(L,1); return true; }
    setInstructionBudget(L);
    int status=lua_pcall(L,0,0,0);
    if (status!=LUA_OK) {
        const char* e=lua_tostring(L,-1);
        gameOutput << PRINT_PREFIX << "ERROR: " << escapeField(e?e:"Lua callback error") << "\n";
        lua_pop(L,1);
        return false;
    }
    return true;
}

static std::string takeResult() {
    std::string result=gameOutput.str()+gameCommands.str();
    gameOutput.str(""); gameOutput.clear();
    gameCommands.str(""); gameCommands.clear();
    return result;
}

static jstring toJava(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

static std::unordered_set<std::string> parseCsvSet(const std::string& csv) {
    std::unordered_set<std::string> values;
    size_t start=0;
    while (start<csv.size()) {
        size_t end=csv.find(',',start);
        if (end==std::string::npos) end=csv.size();
        if (end>start) values.insert(csv.substr(start,end-start));
        start=end+1;
    }
    return values;
}
static void setBoolSetTable(lua_State* L, const char* name,
                            const std::unordered_set<std::string>& values) {
    lua_newtable(L);
    for (const auto& value: values) {
        lua_pushboolean(L,1);
        lua_setfield(L,-2,value.c_str());
    }
    lua_setglobal(L,name);
}
static void applyInput(const std::string& keys, const std::string& actions,
                       float ax, float ay, float px, float py, bool down) {
    bool wasPointerDown=pointerIsDown;
    gAxisX=ax; gAxisY=ay; pointerX=px; pointerY=py;
    pointerPressed = down && !wasPointerDown;
    pointerReleased = !down && wasPointerDown;
    pointerIsDown=down;
    if (gameState) {
        auto currentKeys=parseCsvSet(keys);
        auto currentActions=parseCsvSet(actions);
        std::unordered_set<std::string> pressed, released;
        for (const auto& key: currentKeys) if (!previousKeys.count(key)) pressed.insert(key);
        for (const auto& key: previousKeys) if (!currentKeys.count(key)) released.insert(key);
        for (const auto& id: currentActions) if (!previousActions.count(id)) pressed.insert(id);
        for (const auto& id: previousActions) if (!currentActions.count(id)) released.insert(id);
        if (pointerPressed) { pressed.insert("pointer"); pressed.insert("touch"); }
        if (pointerReleased) { released.insert("pointer"); released.insert("touch"); }
        setCsvTable(gameState,"__aether_keys",keys);
        setCsvTable(gameState,"__aether_actions",actions);
        setBoolSetTable(gameState,"__aether_pressed",pressed);
        setBoolSetTable(gameState,"__aether_released",released);
        previousKeys=std::move(currentKeys);
        previousActions=std::move(currentActions);
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_nativeEngineStatus(JNIEnv* env, jclass) {
    return env->NewStringUTF("AETHER MINE Lua 5.4 V3 Input + Graphics Engine Ready");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_runLua(JNIEnv* env, jclass, jstring source) {
    if (!source) return env->NewStringUTF("ERROR: Empty source.");
    const char* chars=env->GetStringUTFChars(source,nullptr);
    if (!chars) return env->NewStringUTF("ERROR: Cannot read source.");
    std::string code(chars); env->ReleaseStringUTFChars(source,chars);
    std::lock_guard<std::mutex> lock(luaMutex);
    lua_State* L=luaL_newstate();
    if (!L) return env->NewStringUTF("ERROR: Cannot create Lua state.");
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    restrictLibraries(L); setInstructionBudget(L);
    int status=luaL_loadbuffer(L,code.c_str(),code.size(),"AETHER_MINE");
    if (status==LUA_OK) status=lua_pcall(L,0,LUA_MULTRET,0);
    std::string result;
    if (status!=LUA_OK) {
        const char* e=lua_tostring(L,-1);
        result=std::string("ERROR: ")+(e?e:"Unknown Lua Error");
        lua_pop(L,1);
    } else {
        std::string captured=gameOutput.str();
        size_t pos=0;
        while (pos<captured.size()) {
            size_t end=captured.find('\n',pos);
            if (end==std::string::npos) end=captured.size();
            std::string line=captured.substr(pos,end-pos);
            if (line.rfind(PRINT_PREFIX,0)==0) {
                if (!result.empty()) result += "\n";
                result += line.substr(std::string(PRINT_PREFIX).size());
            }
            pos=end+1;
        }
        if (result.empty()) result="Program finished successfully.";
    }
    lua_close(L); gameOutput.str(""); gameOutput.clear();
    return toJava(env,result);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_runLuaGame(JNIEnv* env, jclass, jstring source) {
    if (!source) return env->NewStringUTF("@PRINT\tERROR: Empty game source.\n");
    const char* chars=env->GetStringUTFChars(source,nullptr);
    if (!chars) return env->NewStringUTF("@PRINT\tERROR: Cannot read source.\n");
    std::string code(chars); env->ReleaseStringUTFChars(source,chars);
    std::lock_guard<std::mutex> lock(luaMutex);
    if (gameState) { lua_close(gameState); gameState=nullptr; }
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    gAxisX=gAxisY=pointerX=pointerY=0; pointerIsDown=false;
    pointerPressed=pointerReleased=false;
    previousKeys.clear(); previousActions.clear();
    gameState=luaL_newstate();
    if (!gameState) return env->NewStringUTF("@PRINT\tERROR: Cannot create Lua game state.\n");
    restrictLibraries(gameState);
    setCsvTable(gameState,"__aether_keys","");
    setCsvTable(gameState,"__aether_actions","");
    setBoolSetTable(gameState,"__aether_pressed",{});
    setBoolSetTable(gameState,"__aether_released",{});
    setInstructionBudget(gameState);
    int status=luaL_loadbuffer(gameState,code.c_str(),code.size(),"AETHER_MINE_GAME");
    if (status==LUA_OK) status=lua_pcall(gameState,0,0,0);
    if (status!=LUA_OK) {
        const char* e=lua_tostring(gameState,-1);
        gameOutput << PRINT_PREFIX << "ERROR: " << escapeField(e?e:"Lua load error") << "\n";
        lua_pop(gameState,1);
    } else {
        callLuaFunction(gameState,"setup");
        callLuaFunction(gameState,"draw");
    }
    return toJava(env,takeResult());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_gameFrame(JNIEnv* env, jclass, jstring keys) {
    std::string keyText;
    if (keys) {
        const char* p=env->GetStringUTFChars(keys,nullptr);
        if (p) { keyText=p; env->ReleaseStringUTFChars(keys,p); }
    }
    std::lock_guard<std::mutex> lock(luaMutex);
    if (!gameState) return env->NewStringUTF("@PRINT\tERROR: No game is running.\n");
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    applyInput(keyText,"",0,0,0,0,false);
    callLuaFunction(gameState,"update");
    callLuaFunction(gameState,"draw");
    return toJava(env,takeResult());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_gameFrameInput(JNIEnv* env, jclass,
        jstring keys, jfloat ax, jfloat ay, jfloat px, jfloat py,
        jboolean down, jstring actions) {
    std::string keyText, actionText;
    if (keys) {
        const char* p=env->GetStringUTFChars(keys,nullptr);
        if (p) { keyText=p; env->ReleaseStringUTFChars(keys,p); }
    }
    if (actions) {
        const char* p=env->GetStringUTFChars(actions,nullptr);
        if (p) { actionText=p; env->ReleaseStringUTFChars(actions,p); }
    }
    std::lock_guard<std::mutex> lock(luaMutex);
    if (!gameState) return env->NewStringUTF("@PRINT\tERROR: No game is running.\n");
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    applyInput(keyText,actionText,ax,ay,px,py,down==JNI_TRUE);
    callLuaFunction(gameState,"update");
    callLuaFunction(gameState,"draw");
    return toJava(env,takeResult());
}

extern "C" JNIEXPORT void JNICALL
Java_com_aethermine_lua_MainActivity_stopLuaGame(JNIEnv*, jclass) {
    std::lock_guard<std::mutex> lock(luaMutex);
    if (gameState) { lua_close(gameState); gameState=nullptr; }
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
}
