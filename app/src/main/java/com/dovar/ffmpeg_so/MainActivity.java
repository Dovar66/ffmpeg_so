package com.dovar.ffmpeg_so;

import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    String path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES).getPath()+ File.separator+"vcamera";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
//        System.loadLibrary("postproc-54");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avcodec-57");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        final TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switch (i % 4) {
                    case 0:
                        tv.setText(urlprotocolinfo());
                        break;
                    case 1:
                        tv.setText(avformatinfo());
                        break;
                    case 2:
                        tv.setText(avcodecinfo());
                        break;
                    case 3:
                        tv.setText(avfilterinfo());
                        break;
                    default:
                        break;
                }
                i++;
            }
        });
        final SurfaceView mSurfaceView = findViewById(R.id.surface);
        mSurfaceView.getHolder().setFormat(PixelFormat.RGBA_8888);
        mSurfaceView.postDelayed(new Runnable() {
            @Override
            public void run() {
                new Thread(new Runnable() {
                    @Override
                    public void run() {

                        startPlayPure(path + "/finish.mp4", mSurfaceView.getHolder().getSurface());
                    }
                }).start();
            }
        }, 5000);
    }

    int i = 0;


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native String urlprotocolinfo();

    public native String avformatinfo();

    public native String avcodecinfo();

    public native String avfilterinfo();

//    public native String muxing();

    public native void startPlay(String videoPath, Surface mSurface);
    public native void startPlayPure(String videoPath, Surface mSurface);

}
