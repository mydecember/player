package com.xm.player;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.lang.ref.WeakReference;

public class MiPlayer {
    private final static String TAG = "MiPlayer";
    private long mPlayer = 0;
    private String mPath;
    private SurfaceHolder mSurfaceHolder;

    public MiPlayer() {
        mPlayer = _nativeSetup(new WeakReference<MiPlayer>(this));
        Log.i(TAG, "get player "+mPlayer);
    }

    public void setDataSource(String path) {
        mPath = path;
        _setDataSource(mPlayer, path);
    }

    public void setDisplay(SurfaceHolder sh) {
        mSurfaceHolder = sh;
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        setSurface(surface);
    }

    public void setSurface(Surface surface) {
        mSurfaceHolder = null;
        _setVideoSurface(mPlayer, surface);
    }

    public void start() {_start(mPlayer); }
    public void release() {
        _release(mPlayer);
    }
    public void setScale(float scale) {_scale(mPlayer, scale);}
    public void drag(float x, float y, float xe, float ye) {
        _drag(mPlayer, x, y, xe, ye);
    }

    protected void finalize() throws Throwable {
        super.finalize();
        release();
    }

    private native long _nativeSetup(Object player);
    private native void _setDataSource(long mPlayer, String path);
    private native void _setVideoSurface(long mPlayer, Surface surface);
    private native void _start(long mPlayer);
    private native void _scale(long mPlayer, float scale);
    private native void _release(long mPlayer);
    private native void _drag(long mPlayer, float x, float y, float xe, float ye);
}
