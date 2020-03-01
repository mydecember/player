package com.xm.testcodec;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceView;

import static com.xm.testcodec.MainActivity.TAG;

public class TouchGLSurfaceView extends SurfaceView {
    interface TouchEventListener {
        void onClickUp(float x, float y);
        void onUp(float x, float y);
        void onDown(float x, float y);
        void onDrag(float sx, float sy, float dx, float dy, float cx, float cy);
        void onScale(float dscale);
    }

    private TouchEventListener listener;
    private GestureDetector mGestureDetector;
    private ScaleGestureDetector mScaleGestureDetector;
    private float scrollSX;
    private float scrollSY;
    private boolean scaling = false;

    void setListener(TouchEventListener l) {
        listener = l;
    }


//    @Override
//    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//        Log.i(TAG, "mmmmmmmm " + widthMeasureSpec+ "," + heightMeasureSpec);
//        setMeasuredDimension(1080,1920);
////        mMeasureHelper.doMeasure(widthMeasureSpec, heightMeasureSpec);
////        setMeasuredDimension(mMeasureHelper.getMeasuredWidth(), mMeasureHelper.getMeasuredHeight());
//    }

    private GestureDetector.SimpleOnGestureListener gestureListener = new GestureDetector.SimpleOnGestureListener() {
        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            if (listener != null) {
                listener.onClickUp(e.getX(), e.getY());
            }
            return true;
        }

        @Override
        public boolean onDown(MotionEvent e) {
            if (listener != null) {
                listener.onDown(e.getX(), e.getY());
            }
            return true;
        }

        @Override
        public void onShowPress(MotionEvent e) {
            super.onShowPress(e);
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if (scaling) {
                return true;
            }
            scrollSX = e1.getX();
            scrollSY = e1.getY();
            if (listener != null) {
                listener.onDrag(scrollSX, scrollSY, distanceX, distanceY, e2.getX(), e2.getY());
            }
            return true;
        }
    };

    private ScaleGestureDetector.SimpleOnScaleGestureListener scaleListener = new ScaleGestureDetector.SimpleOnScaleGestureListener() {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            scaling = true;
            if (listener != null) {
                listener.onScale(detector.getScaleFactor());
            }
            return true;
        }
    };

    private void init(Context context) {
        mGestureDetector = new GestureDetector(context, gestureListener);
        mGestureDetector.setIsLongpressEnabled(false);
        mScaleGestureDetector = new ScaleGestureDetector(context, scaleListener);
        mScaleGestureDetector.setQuickScaleEnabled(false);
    }

    public TouchGLSurfaceView(Context context) {
        super(context);
        init(context);
    }

    public TouchGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean ret = false;
        if (event.getActionMasked() == MotionEvent.ACTION_UP) {
            scaling = false;
            if (listener != null) {
                listener.onUp(event.getX(), event.getY());
            }
        }
        if (event.getPointerCount() > 1) {
            ret = mScaleGestureDetector.onTouchEvent(event);
        } else {
            ret = mGestureDetector.onTouchEvent(event);
        }
        return ret || super.onTouchEvent(event);
    }
}
