package com.dovar.ffmpeg_so;

import android.view.ViewGroup;

import com.dovar.ffmpeg_so.player.PlayState;

/**
 * @Date: 2018/11/1
 * @Author: heweizong
 * @Description:
 */
public class DovaPlayer implements IMediaPlayer {
//    private native void _setVolume(float leftVolume, float rightVolume);

 /*   private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;

    public DovaPlayer(@NonNull Context mContext) {
        this.mSurfaceView = new SurfaceView(mContext);
        init();
    }

    private void init() {
        MediaPlayer mMediaPlayer;
        mMediaPlayer.setDataSource();
        mSurfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {

            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }*/

    @Override
    public void setPlayView(ViewGroup mPlayView) {

    }

    @Override
    public void start() {

    }

    @Override
    public void pause() {

    }

    @Override
    public void resume() {

    }

    @Override
    public void stop() {

    }

    @Override
    public void destroy(boolean clearFrame) {

    }

    @Override
    public void seekTo(long milliSec) {

    }

    @Override
    public PlayState getPlayState() {
        return null;
    }

    @Override
    public void setPlayUrl(String playUrl) {

    }

    @Override
    public void setVolume(float percent) {
        if (percent > 1.0f) {
            percent = 1.0f;
        } else if (percent < 0.0f) {
            percent = 0.0f;
        }


    }

    @Override
    public long getCurrentPosition() {
        return 0;
    }

    @Override
    public long getDuration() {
        return 0;
    }

    @Override
    public void setRate(float rate) {

    }

    @Override
    public void setLooping(boolean loop) {

    }

    @Override
    public boolean isLooping() {
        return false;
    }
}
