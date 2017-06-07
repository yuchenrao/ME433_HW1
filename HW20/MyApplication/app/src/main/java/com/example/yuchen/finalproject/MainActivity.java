package com.example.yuchen.finalproject;

import android.Manifest;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.view.WindowManager;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.SeekBar;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import static android.graphics.Color.blue;
import static android.graphics.Color.green;
import static android.graphics.Color.red;
import static android.graphics.Color.rgb;

import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

public class MainActivity extends Activity implements TextureView.SurfaceTextureListener {
    private Camera mCamera;
    private TextureView mTextureView;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
    private Canvas canvas = new Canvas(bmp);
    private Paint paint1 = new Paint();
    private TextView mTextView;

    private UsbManager manager;
    private UsbSerialPort sPort;
    private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();
    private SerialInputOutputManager mSerialIoManager;

    static long prevtime = 0; // for FPS calculation
    int threshT = 0; // brightness detection threshold
    int threshR = 0; // gray detection threshold
    int COM = 0;
    int valuel = 0;
    int valuer = 0;
    int COM1 = 0;
    int COM2 = 0;
//    int error = 0;
//    int MAX_DUTY = 600;
//    int kp = 1;
//    ScrollView myScrollView;
    TextView myTextView2;
//    TextView myTextView3;

    SeekBar myControl;
    SeekBar myControl2;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // keeps the screen from turning off

        mTextView = (TextView) findViewById(R.id.cameraStatus);
        myControl = (SeekBar) findViewById(R.id.seek1);
        myControl2 = (SeekBar) findViewById(R.id.seek2);
//        myScrollView = (ScrollView) findViewById(R.id.ScrollView01);
        myTextView2 = (TextView) findViewById(R.id.textView02);
//        myTextView3 = (TextView) findViewById(R.id.textView03);

        setMyControlListener();
        setMyControl2Listener();

        // see if the app has permission to use the camera
//        ActivityCompat.requestPermissions(MainActivity.this, new String[]{Manifest.permission.CAMERA}, 1);
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            mSurfaceView = (SurfaceView) findViewById(R.id.surfaceview);
            mSurfaceHolder = mSurfaceView.getHolder();

            mTextureView = (TextureView) findViewById(R.id.textureview);
            mTextureView.setSurfaceTextureListener(this);

            // set the paintbrush for writing text on the image
            paint1.setColor(0xffff0000); // red
            paint1.setTextSize(24);

            mTextView.setText("started camera");
        } else {
            mTextView.setText("no camera permissions");
        }

        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
    }

    private void setMyControlListener() {
        myControl.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            int progressChanged = 0;

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChanged = progress;
                threshT = progress;   // use bar to control the thresh
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    private void setMyControl2Listener() {
        myControl2.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            int progressChanged = 0;

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChanged = progress;
                threshR = progress;   // use bar to control the thresh
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    private final SerialInputOutputManager.Listener mListener =
            new SerialInputOutputManager.Listener() {
                @Override
                public void onRunError(Exception e) {

                }

                @Override
                public void onNewData(final byte[] data) {
                    MainActivity.this.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            MainActivity.this.updateReceivedData(data);
                        }
                    });
                }
            };

    @Override
    protected void onPause(){
        super.onPause();
        stopIoManager();
        if(sPort != null){
            try{
                sPort.close();
            } catch (IOException e){ }
            sPort = null;
        }
        finish();
    }

    @Override
    protected void onResume() {
        super.onResume();

        ProbeTable customTable = new ProbeTable();
        customTable.addProduct(0x04D8,0x000A, CdcAcmSerialDriver.class);
        UsbSerialProber prober = new UsbSerialProber(customTable);

        final List<UsbSerialDriver> availableDrivers = prober.findAllDrivers(manager);

        if(availableDrivers.isEmpty()) {
            //check
            return;
        }

        UsbSerialDriver driver = availableDrivers.get(0);
        sPort = driver.getPorts().get(0);

        if (sPort == null){
            //check
        }else{
            final UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
            UsbDeviceConnection connection = usbManager.openDevice(driver.getDevice());
            if (connection == null){
                //check
                PendingIntent pi = PendingIntent.getBroadcast(this, 0, new Intent("com.android.example.USB_PERMISSION"), 0);
                usbManager.requestPermission(driver.getDevice(), pi);
                return;
            }

            try {
                sPort.open(connection);
                sPort.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);

            }catch (IOException e) {
                //check
                try{
                    sPort.close();
                } catch (IOException e1) { }
                sPort = null;
                return;
            }
        }
        onDeviceStateChange();
    }

    private void stopIoManager(){
        if(mSerialIoManager != null) {
            mSerialIoManager.stop();
            mSerialIoManager = null;
        }
    }

    private void startIoManager() {
        if(sPort != null){
            mSerialIoManager = new SerialInputOutputManager(sPort, mListener);
            mExecutor.submit(mSerialIoManager);
        }
    }

    private void onDeviceStateChange(){
        stopIoManager();
        startIoManager();
    }

    private void updateReceivedData(byte[] data) {
        //do something with received data

        //for displaying:
        String rxString = null;
        try {
            rxString = new String(data, "UTF-8"); // put the data you got into a string
//            myTextView2.append(rxString);
            myTextView2.setText("the velocity is " + rxString);
//            myScrollView.fullScroll(View.FOCUS_DOWN);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }

    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mCamera = Camera.open();
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(640, 480);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY); // no autofocusing
        parameters.setAutoExposureLock(true); // keep the white balance constant
        mCamera.setParameters(parameters);
        mCamera.setDisplayOrientation(90); // rotate to portrait mode

        try {
            mCamera.setPreviewTexture(surface);
            mCamera.startPreview();
        } catch (IOException ioe) {
            // Something bad happened
        }
    }

    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // Ignored, Camera does all the work for us
    }

    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mCamera.stopPreview();
        mCamera.release();
        return true;
    }

    // the important function
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // every time there is a new Camera preview frame
        mTextureView.getBitmap(bmp);

        final Canvas c = mSurfaceHolder.lockCanvas();
        if (c != null) {
//            int thresh = 0; // comparison value
            int[] pixels = new int[bmp.getWidth()]; // pixels[] is the RGBA data
            int startY = 250; // which row in the bitmap to analyze to read
//            int endY = 400;
//            for (int startY = 50; startY < bmp.getHeight(); startY = startY + 10) {
            bmp.getPixels(pixels, 0, bmp.getWidth(), 0, startY, bmp.getWidth(), 1);
            int sum_mr = 0; // the sum of the mass times the radius
            int sum_m = 0; // the sum of the masses
            for (int i = 0; i < bmp.getWidth(); i++) {
                if (((green(pixels[i]) - red(pixels[i])) > -threshR)&&((green(pixels[i]) - red(pixels[i])) < threshR)&&(green(pixels[i])  > threshT)) {
                    pixels[i] = rgb(1, 1, 1); // set the pixel to almost 100% black

                    sum_m = sum_m + green(pixels[i])+red(pixels[i])+blue(pixels[i]);
                    sum_mr = sum_mr + (green(pixels[i])+red(pixels[i])+blue(pixels[i]))*i;
                }
            }

                // 200
//            if(sum_m>5) {
//                COM1 = sum_mr / sum_m;
//                canvas.drawCircle(COM1, startY, 3, paint1);
//            }
//            else {
//                COM1 = 0;
//            }
//
//            bmp.getPixels(pixels, 0, bmp.getWidth(), 0, endY, bmp.getWidth(), 1);
//            int sum_mr2 = 0; // the sum of the mass times the radius
//            int sum_m2 = 0; // the sum of the masses
//            for (int i = 0; i < bmp.getWidth(); i++) {
//                if (((green(pixels[i]) - red(pixels[i])) > -threshR)&&((green(pixels[i]) - red(pixels[i])) < threshR)&&(green(pixels[i])  > threshT)) {
//                    pixels[i] = rgb(1, 1, 1); // set the pixel to almost 100% black
//
//                    sum_m2 = sum_m2 + green(pixels[i])+red(pixels[i])+blue(pixels[i]);
//                    sum_mr2 = sum_mr2 + (green(pixels[i])+red(pixels[i])+blue(pixels[i]))*i;
//                }
//            }
//            //400
//            if(sum_m2>5) {
//                COM2 = sum_mr2 / sum_m2;
//                canvas.drawCircle(COM2, endY, 3, paint1);
//            }
//            else {
//                COM2 = 0;
//            }
//
//
//            if (COM1 < 0){
//                COM1 = 0;
//            }
//            else if (COM1 > 600){
//                COM1 = 600;
//            }
//
//            if (COM2 < 0){
//                COM2 = 0;
//            }
//            else if (COM2 > 650){
//                COM2 = 650;
//            }
//
//            COM = COM1 - COM2;
//
//            if (COM < -50){
//                if (COM1 == 0){
//                    valuer = 500;
//                    valuel = 200;
//                }
//                else{
//                    if (COM2 < 50){
//                        valuel = 250;
//                        valuer = 450;
//                    }
//                    else if (COM2 < 120){
//                        valuel = COM2+100;
//                        valuer = COM2+300;
//                    }
//                    else if (COM2 < 280){
//                        valuel = COM2+100;
//                        valuer = COM2+200;
//                    }
//                    else if (COM2 < 350){
//                        valuer = 400;
//                        valuel = 400;
//                    }
//                    else if (COM2 < 420){
//                        valuel = COM2;
//                        valuer = COM2-100;
//                    }
//                    else if (COM2 < 550){
//                        valuel = COM2;
//                        valuer = COM2-200;
//                    }
//                    else {
//                        valuer = 250;
//                        valuel = 450;
//                    }
//
//                }
//
//            }
//
//            else if (COM > 50){
//                if (COM1 == 0){
//                    valuer = 500;
//                    valuel = 200;
//                }
//                else{
//                    if (COM2 < 50){
//                        valuel = 250;
//                        valuer = 450;
//                    }
//                    else if (COM2 < 120){
//                        valuel = COM2+100;
//                        valuer = COM2+300;
//                    }
//                    else if (COM2 < 280){
//                        valuel = COM2+100;
//                        valuer = COM2+200;
//                    }
//                    else if (COM2 < 350){
//                        valuer = 400;
//                        valuel = 400;
//                    }
//                    else if (COM2 < 420){
//                        valuel = COM2;
//                        valuer = COM2-100;
//                    }
//                    else if (COM < 550){
//                        valuel = COM2;
//                        valuer = COM2-200;
//                    }
//                    else {
//                        valuer = 250;
//                        valuel = 450;
//                    }
//
//                }
//
//            }
//
//            else{
//                valuer = 450;
//                valuel = 450;
//            }
//
//            String sendString = String.valueOf(valuer) + ' ' +  String.valueOf(valuel) + '\n';
//            try {
//                sPort.write(sendString.getBytes(), 10); // 10 is the timeout
//            } catch (IOException e) { }


                 //only use the data if there were a few pixels identified, otherwise you might get a divide by 0 error
                if(sum_m>5){
                    COM = sum_mr / sum_m;
                    canvas.drawCircle(COM, startY,3,paint1);

                    if (COM < 0){
                        COM = 0;
                    }
                    else if (COM > 600){
                        COM = 600;
                    }

                    //set velocity for motors
                    if (COM < 50){
                        valuel = 200;
                        valuer = 500;
                    }
                    else if (COM < 120){
                        valuel = COM+100;
                        valuer = COM+400;
                    }
                    else if (COM < 200){
                        valuel = COM+100;
                        valuer = COM+300;
                    }
                    else if (COM < 320){
                        valuel = COM+100;
                        valuer = COM+200;
                    }
                    else if (COM < 360){
                        valuer = 400;
                        valuel = 400;
                    }
                    else if (COM < 420){
                        valuel = COM;
                        valuer = COM-100;
                    }
                    else if (COM < 490){
                        valuel = COM;
                        valuer = COM-200;
                    }
                    else if (COM < 550){
                        valuel = COM;
                        valuer = COM-300;
                    }
                    else {
                        valuer = 200;
                        valuel = 500;
                    }

                    // boundary for velocity
                    if (valuel > 1200){
                        valuel = 1200;
                    }
                    else if (valuel < 0){
                        valuel = 0;
                    }
                    if (valuer > 1200){
                        valuer = 1200;
                    }
                    else if (valuer < 0){
                        valuer = 0;
                    }


                    String sendString = String.valueOf(valuer) + ' ' +  String.valueOf(valuel) + '\n';
                    try {
                        sPort.write(sendString.getBytes(), 10); // 10 is the timeout
                    } catch (IOException e) { }
//                    myTextView2.setText("the position is " + COM);
                }
                else{
                    COM = 0;
                    String sendString = String.valueOf(200) + ' ' +  String.valueOf(200) + '\n';
                    try {
                        sPort.write(sendString.getBytes(), 10); // 10 is the timeout
                    } catch (IOException e) { }

                }

                // update the row
            bmp.setPixels(pixels, 0, bmp.getWidth(), 0, startY, bmp.getWidth(), 1);
//            bmp.setPixels(pixels, 0, bmp.getWidth(), 0, endY, bmp.getWidth(), 1);
            }
//        }

        // write the pos as text
        canvas.drawText("threshT = " + threshT +" threshR = " + threshR, 10, 200, paint1);
        c.drawBitmap(bmp, 0, 0, null);
        mSurfaceHolder.unlockCanvasAndPost(c);

        // calculate the FPS to see how fast the code is running
        long nowtime = System.currentTimeMillis();
        long diff = nowtime - prevtime;
        mTextView.setText("FPS " + 1000 / diff);
        prevtime = nowtime;
    }
}