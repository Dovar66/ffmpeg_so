package com.dovar.ffmpeg_so;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    String path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES).getPath() + File.separator + "vcamera";

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
        SurfaceView mSurfaceView = findViewById(R.id.surface);
        mSurfaceView.getHolder().addCallback(this);
    }

    int i = 0;

    public native int setup(String filePath, Object surface);

    public native int play();

    public native String urlprotocolinfo();

    public native String avformatinfo();

    public native String avcodecinfo();

    public native String avfilterinfo();

    public native void decodeVideo(String videoPath, Surface mSurface);

    public native void decodeAudio(String audioPath);

//    public native void play(String videoPath, Surface mSurface);


    @Override
    public void surfaceCreated(final SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
//                decodeAudio(path + File.separator + "music.mp3");
                setup(path + File.separator + "test.mp4", holder.getSurface());
                play();
            }
        }).start();

       /* new Thread(new Runnable() {
            @Override
            public void run() {
                decodeVideo(path + File.separator + "test.mp4", holder.getSurface());
            }
        }).start();*/
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }


    /**
     * 创建一个AudioTrack对象
     *
     * @param sampleRate 采样率
     * @param channels   声道布局
     */
    public AudioTrack createAudioTrack(int sampleRate, int channels) {
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int channelConfig;
        if (channels == 1) {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

        return new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
    }
}
