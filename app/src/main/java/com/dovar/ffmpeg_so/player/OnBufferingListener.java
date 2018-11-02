package com.dovar.ffmpeg_so.player;

/**
 * @Date: 2018/11/1
 * @Author: heweizong
 * @Description:
 */
public interface OnBufferingListener {
    /**
     * @param var1 缓冲进度
     * @param var2 视频总时长
     */
    void onBufferingUpdate(long var1, long var2);
}
