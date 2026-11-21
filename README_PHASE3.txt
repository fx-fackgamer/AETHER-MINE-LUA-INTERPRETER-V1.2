AETHER MINE LUA GAME STUDIO V2.1 — PHASE 3

এই Phase-এ index.html-কে Phase 2 Native Input/UI API-এর সঙ্গে যুক্ত করা হয়েছে।

ফাইল বসানোর জায়গা:
app/src/main/assets/index.html

যা যুক্ত হয়েছে:
- Run Page fullscreen layout এবং Close/Reload/Stop
- Run Page-এর Folder Settings-এর মধ্যে j.j, ABRL, j.A, m.m, C.C, K.B
- Portrait-এ Run/Settings উপর-নিচ split; Landscape-এ Settings বাম পাশে, Run ডান পাশে
- Touch joystick, Jump/Action এবং directional touch buttons
- Keyboard input এবং pointer coordinates
- Native frameInput(keys, axisX, axisY, pointerX, pointerY, pointerDown, actions) ব্যবহার
- Lua @UI panel/text/button command Canvas-এ render

গুরুত্বপূর্ণ:
এই HTML ফাইলটি Phase 2-এর MainActivity.java এবং native-lib.cpp-এর সঙ্গে ব্যবহার করতে হবে। তিনটি ফাইল একই V2.1 branch-এ থাকতে হবে। Native/Gradle build এখনও ডিভাইসে পরীক্ষা করা হয়নি; তাই APK build করে পরীক্ষা করার পরই সফল বলে ধরা যাবে।

প্রস্তাবিত Commit Message:
Connect V2.1 Run UI with Native Touch Input
