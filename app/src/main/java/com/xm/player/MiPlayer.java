package com.xm.player;

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


    public void release() {
        _release(mPlayer);
    }

    protected void finalize() throws Throwable {
        super.finalize();
        release();
    }

    private native long _nativeSetup(Object player);
    private native int _setDataSource(long mPlayer, String path);
    private native void _setVideoSurface(long mPlayer, Surface surface);
    private native void _release(long mPlayer);
}
