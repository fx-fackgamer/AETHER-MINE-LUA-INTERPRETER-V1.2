
#include <jni.h>
#include <string>
#include <sstream>
#include <mutex>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static std::mutex luaMutex;

// Capture print() output
static int capturePrint(lua_State* L) {

    std::ostringstream* output =
        static_cast<std::ostringstream*>(
            lua_touserdata(L, lua_upvalueindex(1))
        );

    int n = lua_gettop(L);

    for (int i = 1; i <= n; i++) {

        if (i > 1) {
            (*output) << "\t";
        }

        size_t len = 0;

        const char* value =
            luaL_tolstring(L, i, &len);

        if (value) {
            output->write(
                value,
                static_cast<std::streamsize>(len)
            );
        }

        lua_pop(L, 1);
    }

    (*output) << "\n";

    return 0;
}

// Limit excessive Lua instructions
static void executionLimit(lua_State* L,
                           lua_Debug* debug) {

    (void)debug;

    int* remaining =
        static_cast<int*>(lua_getextraspace(L));

    (*remaining)--;

    if (*remaining <= 0) {
        luaL_error(L, "Execution limit exceeded.");
    }
}

// Native Engine Status
extern "C"
JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_nativeEngineStatus(
    JNIEnv* env,
    jclass
) {

    return env->NewStringUTF(
        "AETHER MINE Lua 5.4 Native Engine Ready"
    );
}

// Execute Lua Code
extern "C"
JNIEXPORT jstring JNICALL
Java_com_aethermine_lua_MainActivity_runLua(
    JNIEnv* env,
    jclass,
    jstring source
) {

    if (source == nullptr) {
        return env->NewStringUTF(
            "ERROR: Empty source."
        );
    }

    const char* chars =
        env->GetStringUTFChars(source, nullptr);

    if (chars == nullptr) {
        return env->NewStringUTF(
            "ERROR: Cannot read source."
        );
    }

    std::string code(chars);

    env->ReleaseStringUTFChars(source, chars);

    std::lock_guard<std::mutex> lock(luaMutex);

    lua_State* L = luaL_newstate();

    if (L == nullptr) {
        return env->NewStringUTF(
            "ERROR: Cannot create Lua state."
        );
    }

    // Load standard Lua libraries
    luaL_openlibs(L);

    // Restrict access to system and file libraries
    const char* restricted[] = {
        "io",
        "os",
        "debug",
        "package",
        "require",
        "dofile",
        "loadfile"
    };

    for (const char* name : restricted) {
        lua_pushnil(L);
        lua_setglobal(L, name);
    }

    // Output capture
    std::ostringstream output;

    lua_pushlightuserdata(L, &output);

    lua_pushcclosure(
        L,
        capturePrint,
        1
    );

    lua_setglobal(L, "print");

    // Instruction limit
    int remaining = 2000;

    *static_cast<int*>(lua_getextraspace(L)) =
        remaining;

    lua_sethook(
        L,
        executionLimit,
        LUA_MASKCOUNT,
        1000
    );

    // Load Lua source
    int status = luaL_loadbuffer(
        L,
        code.c_str(),
        code.size(),
        "AETHER_MINE"
    );

    // Execute Lua source
    if (status == LUA_OK) {

        status = lua_pcall(
            L,
            0,
            LUA_MULTRET,
            0
        );
    }

    // Error handling
    if (status != LUA_OK) {

        const char* error =
            lua_tostring(L, -1);

        output << "ERROR: ";

        output << (
            error ? error : "Unknown Lua Error"
        );

        output << "\n";
    }

    lua_close(L);

    std::string result = output.str();

    if (result.empty() && status == LUA_OK) {
        result =
            "Program finished successfully.";
    }

    return env->NewStringUTF(result.c_str());
}
