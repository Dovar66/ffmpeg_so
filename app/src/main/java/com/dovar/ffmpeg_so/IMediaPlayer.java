package com.dovar.ffmpeg_so;

import android.view.ViewGroup;

import com.dovar.ffmpeg_so.player.PlayState;

/**
 * @Date: 2018/11/1
 * @Author: heweizong
 * @Description: 播放接口
 */
public interface IMediaPlayer {

    /**
     * 设置播放视图的父容器
     */
    void setPlayView(ViewGroup mPlayView);

    //开始播放
    void start();

    //暂停
    void pause();

    //暂停之后的恢复播放
    void resume();

    //停止播放
    void stop();

    /**
     * 销毁播放器
     *
     * @param clearFrame 清除停止播放时的最后一帧画面
     */
    void destroy(boolean clearFrame);

    /**
     * 快进
     *
     * @param milliSec 目标进度，单位毫秒
     */
    void seekTo(long milliSec);

    /**
     * 获取播放器当前状态
     */
    PlayState getPlayState();

    /**
     * 设置播放流地址
     * 设置地址后立即开始加载但不播放，{@link #start()}调用后才开始播放画面
     */
    void setPlayUrl(String playUrl);

    /**
     * 设置音量
     *
     * @param percent 0.0~1.0
     */
    void setVolume(float percent);

    /**
     * 获取当前播放进度 单位毫秒
     */
    long getCurrentPosition();

    /**
     * 获取视频总时长 单位毫秒
     */
    long getDuration();

    /**
     * 设置倍速播放
     *
     * @param rate 倍率
     */
    void setRate(float rate);

    /**
     * 设置是否循环播放
     */
    void setLooping(boolean loop);

    boolean isLooping();
}
