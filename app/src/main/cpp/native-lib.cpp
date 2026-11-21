#include <jni.h>
#include <string>
#include <sstream>
#include <mutex>
#include <cmath>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static std::mutex luaMutex;
static lua_State* gameState = nullptr;
static std::ostringstream gameOutput;
static std::ostringstream gameCommands;
static int instructionsLeft = 120000;
static const char* GAME_PREFIX = "@GAME\t";
static const char* PRINT_PREFIX = "@PRINT\t";

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
    instructionsLeft = 120000;
    *static_cast<int*>(lua_getextraspace(L)) = instructionsLeft;
    lua_sethook(L, executionLimit, LUA_MASKCOUNT, 1000);
}

static int gameCreate(lua_State* L) {
    int w = static_cast<int>(luaL_checkinteger(L, 1));
    int h = static_cast<int>(luaL_checkinteger(L, 2));
    if (w < 1 || w > 4096 || h < 1 || h > 4096)
        return luaL_error(L, "game.create width/height must be 1..4096");
    emitGame("create\t" + std::to_string(w) + "\t" + std::to_string(h));
    return 0;
}
static int gameBackground(lua_State* L) {
    const char* c = luaL_checkstring(L, 1);
    emitGame("background\t" + escapeField(c));
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
    int size=static_cast<int>(luaL_optinteger(L,4,24));
    const char* c=luaL_optstring(L,5,"#ffffff");
    emitGame("text\t"+escapeField(t)+"\t"+std::to_string(x)+"\t"+
             std::to_string(y)+"\t"+std::to_string(size)+"\t"+escapeField(c));
    return 0;
}
static int gameKey(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_getglobal(L, "__aether_keys");
    if (!lua_istable(L, -1)) { lua_pop(L,1); lua_pushboolean(L,0); return 1; }
    lua_getfield(L, -1, key);
    bool down = lua_toboolean(L, -1) != 0;
    lua_pop(L, 2);
    lua_pushboolean(L, down);
    return 1;
}
static void registerGameApi(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L, gameCreate); lua_setfield(L,-2,"create");
    lua_pushcfunction(L, gameBackground); lua_setfield(L,-2,"background");
    lua_pushcfunction(L, gameRect); lua_setfield(L,-2,"rect");
    lua_pushcfunction(L, gameCircle); lua_setfield(L,-2,"circle");
    lua_pushcfunction(L, gameLine); lua_setfield(L,-2,"line");
    lua_pushcfunction(L, gameText); lua_setfield(L,-2,"text");
    lua_pushcfunction(L, gameKey); lua_setfield(L,-2,"key");
    lua_setglobal(L,"game");
}
static void restrictLibraries(lua_State* L) {
    luaL_openlibs(L);
    const char* restricted[]={"io","os","debug","package","require","dofile","loadfile"};
    for (const char* name:restricted) { lua_pushnil(L); lua_setglobal(L,name); }
    lua_pushcfunction(L,capturePrint); lua_setglobal(L,"print");
    registerGameApi(L);
}
static void setKeys(lua_State* L, const std::string& csv) {
    lua_newtable(L);
    size_t start=0;
    while (start < csv.size()) {
        size_t end=csv.find(',',start);
        if (end==std::string::npos) end=csv.size();
        std::string key=csv.substr(start,end-start);
        if (!key.empty()) { lua_pushboolean(L,1); lua_setfield(L,-2,key.c_str()); }
        start=end+1;
    }
    lua_setglobal(L,"__aether_keys");
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
static jstring toJava(JNIEnv* env,const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_nativeEngineStatus(JNIEnv* env,jclass) {
    return env->NewStringUTF("AETHER MINE Lua 5.4 Native Engine Ready");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_runLua(JNIEnv* env,jclass,jstring source) {
    if (!source) return env->NewStringUTF("ERROR: Empty source.");
    const char* chars=env->GetStringUTFChars(source,nullptr);
    if (!chars) return env->NewStringUTF("ERROR: Cannot read source.");
    std::string code(chars); env->ReleaseStringUTFChars(source,chars);
    std::lock_guard<std::mutex> lock(luaMutex);
    lua_State* L=luaL_newstate();
    if (!L) return env->NewStringUTF("ERROR: Cannot create Lua state.");
    gameOutput.str(""); gameOutput.clear();
    gameCommands.str(""); gameCommands.clear();
    restrictLibraries(L);
    setInstructionBudget(L);
    int status=luaL_loadbuffer(L,code.c_str(),code.size(),"AETHER_MINE");
    if (status==LUA_OK) status=lua_pcall(L,0,LUA_MULTRET,0);
    if (status!=LUA_OK) {
        const char* e=lua_tostring(L,-1);
        gameOutput << PRINT_PREFIX << "ERROR: "
                   << escapeField(e?e:"Unknown Lua Error") << "\\n";
    }
    std::string captured=gameOutput.str();
    std::string result;
    size_t pos=0;
    while (pos<captured.size()) {
        size_t end=captured.find('\\n',pos);
        if (end==std::string::npos) end=captured.size();
        std::string line=captured.substr(pos,end-pos);
        if (line.rfind(PRINT_PREFIX,0)==0) {
            if (!result.empty()) result += "\\n";
            result += line.substr(std::string(PRINT_PREFIX).size());
        }
        pos=end+1;
    }
    if (result.empty() && status==LUA_OK) result="Program finished successfully.";
    lua_close(L);
    gameOutput.str(""); gameOutput.clear();
    return toJava(env,result);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_runLuaGame(JNIEnv* env,jclass,jstring source) {
    if (!source) return env->NewStringUTF("ERROR: Empty game source.");
    const char* chars=env->GetStringUTFChars(source,nullptr);
    if (!chars) return env->NewStringUTF("ERROR: Cannot read game source.");
    std::string code(chars); env->ReleaseStringUTFChars(source,chars);
    std::lock_guard<std::mutex> lock(luaMutex);
    if (gameState) { lua_close(gameState); gameState=nullptr; }
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    gameState=luaL_newstate();
    if (!gameState) return env->NewStringUTF("@PRINT\tERROR: Cannot create Lua game state.\n");
    restrictLibraries(gameState);
    registerGameApi(gameState);
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
Java_com_aethermine_lua_MainActivity_gameFrame(JNIEnv* env,jclass,jstring keys) {
    std::string keyText;
    if (keys) {
        const char* p=env->GetStringUTFChars(keys,nullptr);
        if (p) { keyText=p; env->ReleaseStringUTFChars(keys,p); }
    }
    std::lock_guard<std::mutex> lock(luaMutex);
    if (!gameState) return env->NewStringUTF("@PRINT\tERROR: No game is running.\n");
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
    setKeys(gameState,keyText);
    callLuaFunction(gameState,"update");
    callLuaFunction(gameState,"draw");
    return toJava(env,takeResult());
}

extern "C" JNIEXPORT void JNICALL
Java_com_aethermine_lua_MainActivity_stopLuaGame(JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(luaMutex);
    if (gameState) { lua_close(gameState); gameState=nullptr; }
    gameOutput.str(""); gameOutput.clear(); gameCommands.str(""); gameCommands.clear();
}
