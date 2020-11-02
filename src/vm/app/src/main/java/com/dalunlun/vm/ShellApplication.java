package com.dalunlun.vm;

import android.app.Application;

public class ShellApplication extends Application {
    @Override
    public void onCreate() {
        System.loadLibrary("vm");
    }
}
