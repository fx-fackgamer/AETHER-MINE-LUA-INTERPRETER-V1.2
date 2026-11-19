# AETHER MINE LUA INTERPRETER V1 — Starter

এই starter project-এ WebView UI এবং JNI/C++ bridge-এর ভিত্তি আছে।
**এখনো Lua 5.4 source code যুক্ত করা হয়নি**, তাই Lua engine বসানোর আগে Run চাপলে native engine unavailable দেখাবে।

## ধাপ ১: Project তৈরি ও খোলা
1. ZIP extract করো।
2. `AETHER_MINE_LUA_INTERPRETER_V1` ফোল্ডারটি GitHub repository-তে upload করো অথবা Android Studio-তে Open করো।
3. Android Studio-তে Gradle Sync সম্পন্ন করো।
4. `app/src/main/cpp/lua/README_BN.txt` অনুসরণ করে Lua 5.4-এর official source files `lua` ফোল্ডারে রাখো।

## ধাপ ২: Lua 5.4 source যোগ করা
Lua-এর official website থেকে Lua 5.4.x source archive সংগ্রহ করে extract করো।
`src` ফোল্ডারের `.c` ও `.h` ফাইলগুলো `app/src/main/cpp/lua/`-এ কপি করো।
`lua.c` এবং `luac.c` বাদ দাও; Android app নিজস্ব JNI entry point ব্যবহার করে।

## ধাপ ৩: Build
Android Studio-তে Sync করে Build > Make Project করো।
তারপর Android device-এ Run করো।

## গুরুত্বপূর্ণ
- Lua 5.4 runtime C API-তে চলে; C++ bridge JNI দিয়ে Java/Kotlin-এ যুক্ত।
- WebView UI-তে JavaScript bridge সীমিত রাখা হয়েছে।
- V1 starter-এ `print()` output এবং basic execution result bridge করা হবে।
- অপরিচিত/অনিরাপদ Lua script চালালে app-এর data বা performance ঝুঁকিতে পড়তে পারে। V1-এ script execution সীমিত ও local রাখো।
