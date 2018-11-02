package com.dovar.ffmpeg_so.player;

/**
 * @Date: 2018/11/1
 * @Author: heweizong
 * @Description: 播放状态
 */
public enum PlayState {
    IDLE,
    PREPARING,
    PREPARED,
    PLAYING,
    BUFFERING,
    PAUSED,
    COMPLETED,
    ERROR,
    RECONNECTING,
    PLAYING_CACHE,
    DESTROYED
}
