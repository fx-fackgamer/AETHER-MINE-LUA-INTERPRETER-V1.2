package com.aethermine.lua;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.JavascriptInterface;
import android.view.View;
import android.view.WindowManager;

public class MainActivity extends Activity {
    private WebView webView;

    static {
        System.loadLibrary("aetherlua");
    }

    public static native String runLua(String source);
    public static native String nativeEngineStatus();
    public static native String runLuaGame(String source);
    public static native String gameFrame(String keys);
    public static native void stopLuaGame();

    public class LuaBridge {
        @JavascriptInterface
        public String run(String source) {
            if (source == null) source = "";
            if (source.length() > 200000) return "ERROR: Script too large.";
            return runLua(source);
        }

        @JavascriptInterface
        public String nativeEngineStatus() {
            return MainActivity.nativeEngineStatus();
        }

        @JavascriptInterface
        public String runGame(String source) {
            if (source == null) source = "";
            if (source.length() > 200000) return "@PRINT\tERROR: Game script too large.\n";
            return runLuaGame(source);
        }

        @JavascriptInterface
        public String frame(String keys) {
            return gameFrame(keys == null ? "" : keys);
        }

        @JavascriptInterface
        public void stopGame() {
            stopLuaGame();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        webView = new WebView(this);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setDomStorageEnabled(true);
        webView.setWebViewClient(new WebViewClient());
        webView.setWebChromeClient(new WebChromeClient());
        webView.addJavascriptInterface(new LuaBridge(), "AetherLua");
        setContentView(webView);
        webView.loadUrl("file:///android_asset/index.html");
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) webView.goBack();
        else super.onBackPressed();
    }

    @Override
    protected void onDestroy() {
        if (webView != null) webView.destroy();
        super.onDestroy();
    }
}
