
package com.aethermine.lua;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.JavascriptInterface;

public class MainActivity extends Activity {

    private WebView webView;

    static {
        System.loadLibrary("aetherlua");
    }

    public static native String runLua(String source);
    public static native String nativeEngineStatus();

    public class LuaBridge {

        @JavascriptInterface
        public String run(String source) {
            if (source == null) source = "";

            if (source.length() > 200000) {
                return "ERROR: Script too large.";
            }

            return runLua(source);
        }

        @JavascriptInterface
        public String engineStatus() {
            return nativeEngineStatus();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        webView = new WebView(this);

        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setDomStorageEnabled(true);

        webView.setWebViewClient(new WebViewClient());
        webView.setWebChromeClient(new WebChromeClient());

        webView.addJavascriptInterface(
            new LuaBridge(), "AetherLua"
        );

        setContentView(webView);

        webView.loadUrl(
            "file:///android_asset/index.html"
        );
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }
}
