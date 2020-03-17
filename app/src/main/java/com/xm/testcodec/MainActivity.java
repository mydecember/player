package com.xm.testcodec;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import com.xm.player.MiPlayer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * if use GLSurfaceview  must implements GLSurfaceView.Renderer
 * if use Surfaceview must implements SurfaceHolder.Callback
 */
public class MainActivity extends AppCompatActivity implements TouchGLSurfaceView.TouchEventListener,  SurfaceHolder.Callback{
    @Override
    protected void onPause() {
        super.onPause();
        //glSurfaceView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    final static String TAG = "codec";
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private Button btnStart;
    private Button btnStart1;
    private TouchGLSurfaceView glSurfaceView;
    private MiPlayer player = new MiPlayer();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //去除title
        requestWindowFeature(Window.FEATURE_NO_TITLE);
//去掉Activity上面的状态栏
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_main);
        checkPermission();
        Log.i(TAG, " RRRRRRRRRRRR onCreate  ");

        // Example of a call to a native method
//        TextView tv = findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
        btnStart = (Button)findViewById(R.id.button);
        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                player.start();
            }
        });

        btnStart1 = (Button)findViewById(R.id.button1);
        btnStart1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startJNI3();
            }
        });

        glSurfaceView = (TouchGLSurfaceView)findViewById(R.id.surfaceView);
        glSurfaceView.setListener(this);
        surfaceViewInit();
    }

    private void glSurfaceInit() {
//        glSurfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
//        glSurfaceView.setEGLContextClientVersion(3);
//        glSurfaceView.setRenderer(this);
//        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    private void surfaceViewInit() {
        glSurfaceView.getHolder().addCallback(this);
    }

    private void checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            String[] permissions = new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.CAMERA}; // 选择你需要申请的权限
            for (int i = 0; i < permissions.length; i++) {
                int state = ContextCompat.checkSelfPermission(this, permissions[i]);
                if (state != PackageManager.PERMISSION_GRANTED) { // 判断权限的状态
                    ActivityCompat.requestPermissions(this, permissions, 200); // 申请权限
                    return;
                }
            }
        }
    }

    // TouchGLSurfaceView.TouchEventListener
    @Override
    public void onClickUp(float x, float y) {
        Log.i(TAG, " click up " + x + "," + y);
    }

    @Override
    public void onUp(float x, float y) {
        Log.i(TAG, " touch down " + x + "," + y);
    }

    @Override
    public void onDown(float x, float y) {
        Log.i(TAG, " touch down " + x + "," + y);
    }

    @Override
    public void onDrag(float sx, float sy, float dx, float dy, float cx, float cy) {
        Log.i(TAG, " drag sx " + sx + " sy " + sy + " dx " + dx + " dy " + dy + " cx " + cx + " cy " + cy);
        player.drag(sx, sy, cx, cy);
    }

    @Override
    public void onScale(float dscale) {
        Log.i(TAG, " scale " + dscale);
        player.setScale(dscale);
    }
    // implements GLSurfaceView.Renderer
//    @Override
//    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
//
//    }
//
//    @Override
//    public void onSurfaceChanged(GL10 gl, int width, int height) {
//
//    }
//
//    @Override
//    public void onDrawFrame(GL10 gl) {
//
//    }

    //  SurfaceHolder.Callback
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, " RRRRRRRRRRRRsurfaceCreated w ");
        player.setDisplay(holder);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, " RRRRRRRRsurfaceChanged w " + width + " h " + height + " format " + format);
        glSurfaceView.getHolder().setFixedSize(width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, " RRRRRRRRRsurfaceDestroyed w ");
        player.setDisplay(null);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native void startJNI();
    public native void startJNI3();

}
