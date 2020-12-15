package com.dalunlun.testapp;

import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        System.out.println("dalunlun");
        System.out.println(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());

        this.test();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public void test() {
        long ret = 0;
        int count = 10;
        for (int i = 1; i < count; i++) {
            ret = this.sum(ret, i);
        }
        System.out.println("ret_1: " + ret);
        System.out.println("ret_2: " + this.sumTo(count - 1));
    }

    private long sum(long a, long b) {
        return a + b;
    }

    long sumTo(long a) {
        if (a == 1) {
            return 1;
        }
        return a + this.sumTo(a - 1);
    }
}