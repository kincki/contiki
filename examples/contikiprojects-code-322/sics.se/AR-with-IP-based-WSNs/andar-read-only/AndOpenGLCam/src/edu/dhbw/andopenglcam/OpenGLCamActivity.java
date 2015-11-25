/**
	Copyright (C) 2009  Tobias Domhan

    This file is part of AndOpenGLCam.

    AndObjViewer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AndObjViewer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AndObjViewer.  If not, see <http://www.gnu.org/licenses/>.
 
 */
package edu.dhbw.andopenglcam;

import java.io.IOException;
import java.util.Date;

import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.ResponseHandler;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.Window;
import android.view.WindowManager;
import android.view.SurfaceHolder.Callback;
import edu.dhbw.andopenglcam.util.IO;

public class OpenGLCamActivity extends Activity implements Callback{
	private GLSurfaceView glSurfaceView;
	private Camera camera;
	private OpenGLCamRenderer renderer;
	private Resources res;
	private CameraPreviewHandler cameraHandler;
	private boolean mPreviewing = false;
	private boolean mPausing = false;
	private MarkerInfo markerInfo = new MarkerInfo(this);
	
	private static final int MAX_NUM_OF_MARKERS = 4;
	
	private static String IpAddress = "193.10.66.60";
	//private static String IpAddress = "130.243.151.10";
	//private static String IpAddress = "193.10.67.40";
	
	public Date lastTime[] = new Date[MAX_NUM_OF_MARKERS];
	public RestServiceData lastServiceData[] = new RestServiceData[MAX_NUM_OF_MARKERS];
	public boolean lastCallResult[] = new boolean[MAX_NUM_OF_MARKERS];
	
	public int currentMarker = -1;
	
	private static final int UPDATE_TIMEOUT =3000;
	private static final int UNSUCCESSFUL_UPDATE_TIMEOUT = 10000;
	
	//public float values[] = new float[2];
	//Date lastTime;
	//RestServiceData lastServiceData;
	
	private static final String INNER_TAG = "HEBEE";
	
	protected static final int TEMP_SERVICE = 0x1;
    protected static final int LIGHT_SERVICE = 0x2;
    protected static final int ENERGY_SERVICE = 0x3;
    protected static final int ALL_SERVICE = 0x4;
    
  //DY MarkerInfo sends message
    public static final int MARKER_NOTIF = 0x10;
    
	private Handler mChildHandler;

	//DY MarkerInfo sends message
	public Handler mMainHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
            	Log.i("HEBEE", "handleMessage: " + msg.what);
            	
				switch (msg.what) {
					case TEMP_SERVICE:
						//values[0] = (float) msg.arg1;
						break;
					case LIGHT_SERVICE:
						break;
					case ALL_SERVICE:
						int marker = msg.arg1;
						lastServiceData[marker] = (RestServiceData) msg.obj;
						lastCallResult[marker] = true;
						Log.i("HEBEE", "ALL SERVICE " + lastServiceData[marker].light);
						break;
					case MARKER_NOTIF:
						currentMarker = msg.arg1;
						notifyCurrentMarker( msg.arg1 );
						break;
					default:
						break;
				}
		        super.handleMessage(msg);
		   } 
    };
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	Log.i("HEBEE", "onCreate");
        super.onCreate(savedInstanceState);
        setFullscreen();
        disableScreenTurnOff();
        //orientation is set via the manifest

        res = getResources();  
        IO.transferFilesToSDCard(res);
        glSurfaceView = new OpenGLCamView(this);
		renderer = new OpenGLCamRenderer(res, markerInfo, this);
		cameraHandler = new CameraPreviewHandler(glSurfaceView, renderer, res, markerInfo);
        glSurfaceView.setRenderer(renderer);
        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        glSurfaceView.getHolder().addCallback(this);
        
        
     // Fire off a thread to do some work that we shouldn't do directly in the UI thread
        Thread t = new Thread() {
            public void run() {
            	
            	/*
                 * You have to prepare the looper before creating the handler.
                 */
                Looper.prepare();
                
                /*
                 * Create the child handler on the child thread so it is bound to the
                 * child thread's message queue.
                 */
                mChildHandler = new Handler() {

                    public void handleMessage(Message msg) {

                    	int marker = msg.arg1;

                    	Log.i(INNER_TAG, "Main->Child marker:" + msg.arg1);
                        	
                        connect(marker);
                        //connect("http://193.10.66.60:8080/temp");
                    }
                };

                Log.i(INNER_TAG, "Child handler is bound to - " + mChildHandler.getLooper().getThread().getName());

                /*
                 * Start looping the message queue of this thread.
                 */
                Looper.loop();
            }

        };
        t.start();
        
        
        for ( int i = 0 ; i < MAX_NUM_OF_MARKERS ; i++ )
    	{
    		lastCallResult[i] = false;
    	}
        
        setContentView(glSurfaceView); 	 
    }
    
    public void connect(int marker)
    {
        HttpClient httpclient = new DefaultHttpClient();
        String url;
        
        switch (marker)
        {
        	case 0:
        		url = "http://" + IpAddress + ":8080/all";
        		break;
        	case 1:
        		url = "http://" + IpAddress + ":8181/all";
        		break;
        	case 2:
        		url = "http://" + IpAddress + ":8282/all";
        		break;
        	default:
        		url = "http://" + IpAddress + ":8080/all";
        		break;
        }
        
        Log.i("HEBEE","Connect to "+url);
 
        // Prepare a request object
        HttpGet httpget = new HttpGet(url);
        	
        try {
        	// Create a response handler
            ResponseHandler<String> responseHandler = new BasicResponseHandler();
            
            String responseBody;
			
				responseBody = httpclient.execute(httpget, responseHandler);
				
				// Examine the response status
	            Log.i("HEBEE","##### DATA From:"+marker+ " RB: " + responseBody);

	            // When HttpClient instance is no longer needed, 
	            // shut down the connection manager to ensure
	            // immediate deallocation of all system resources
	            httpclient.getConnectionManager().shutdown();
	            
	            RestServiceData restData = new RestServiceData();
	            //mHandler.post(mUpdate);
	            Message m = mMainHandler.obtainMessage();
                m.what = ALL_SERVICE;
                m.arg1 = marker;

                String delims = "[;]";
                String[] tokens = responseBody.split(delims);
                for (int i = 0; i < tokens.length; i++)
                {
                    System.out.println(tokens[i]);
                }

                restData.light = Integer.parseInt(tokens[0]);
                restData.temp = Float.parseFloat(tokens[1]);
                restData.cpu = Integer.parseInt(tokens[2]);
        		restData.lpm = Integer.parseInt(tokens[3]);
        		restData.cpuAndlpm = Integer.parseInt(tokens[4]);
        		restData.transmit = Integer.parseInt(tokens[5]);
        		restData.listen = Integer.parseInt(tokens[6]);
        		restData.idle_transmit = Integer.parseInt(tokens[7]);
        		restData.idle_listen = Integer.parseInt(tokens[8]);

                //m.arg1 = Float.valueOf(responseBody).intValue();
        		m.obj = restData;
                mMainHandler.sendMessage(m);
			}
	        catch (ClientProtocolException e) {
				Log.i("HEBEE","ClientProtocolException " + e);
			} catch (IOException e) {
				Log.i("HEBEE","IOException " + e);
			}
			catch (Exception e) {
				Log.i("HEBEE","Exception " + e);
			}
    }
    
   
    
    public void disableScreenTurnOff() {
    	getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
    			WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    
    public void setOrientation()  {
    	setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }
    
    public void setFullscreen() {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }
   
    public void setNoTitle() {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
    } 
    
    @Override
    protected void onPause() {
    	Log.i("HEBEE", "onPause");
    	mPausing = true;
        this.glSurfaceView.onPause();
        
        //DY
        renderer.isTextureInitialized = false;
        
        super.onPause();
    }
    
    

    @Override
    protected void onResume() {
    	Log.i("HEBEE", "onResume");
    	mPausing = false;
    	glSurfaceView.onResume();
        super.onResume();
    }
    
    /* (non-Javadoc)
     * @see android.app.Activity#onStop()
     */
    @Override
    protected void onStop() {
    	Log.i("HEBEE", "onStop");
    	
    	super.onStop();
    	
//    	Context myContext = this;
//    	ActivityManager activityManager = (ActivityManager)myContext.getSystemService(Context.ACTIVITY_SERVICE);
//    	activityManager.restartPackage("edu.dhbw.andopenglcam");
    }
    
    //DY
    @Override
    protected void onDestroy() {
    	Log.i("HEBEE", "onDestroy");
        Log.i("HEBEE", "Stop looping the child thread's message queue");

        /*
         * Remember to stop the looper
         */
        mChildHandler.getLooper().quit();
        
        //DY : fix
        cameraHandler.mConversionHandler.getLooper().quit();

        super.onDestroy();
    }
    
    private void openCamera()  {
    	Log.i("HEBEE", "openCamera");
    	if (camera == null) {
	    	//camera = Camera.open();
    		camera = CameraHolder.instance().open();
    		
	        Parameters params = camera.getParameters();
	        params.setPreviewSize(240,160);
	        //params.setPreviewFrameRate(10);//TODO remove restriction
	        //try to set the preview format
	        params.setPreviewFormat(PixelFormat.YCbCr_420_SP);
	        camera.setParameters(params);
	        if (Integer.parseInt(Build.VERSION.SDK) <= 4) {
	        	//for android 1.5 compatibilty reasons:
				 /*try {
				    camera.setPreviewDisplay(glSurfaceView.getHolder());
				 } catch (IOException e1) {
				        e1.printStackTrace();
				 }*/
	        }
	        
	        //DY
	        cameraHandler.setCam(camera);
	        
	        if(!Config.USE_ONE_SHOT_PREVIEW) {
	        	camera.setPreviewCallback(cameraHandler);	 
	        } else {
	        	camera.setOneShotPreviewCallback(cameraHandler);
	        }
	        try {
				cameraHandler.init(camera);
			} catch (Exception e) {
				//TODO: notify the user
			}
    	}
    }
    
    private void closeCamera() {
    	Log.i("HEBEE", "closeCamera");
        if (camera != null) {
        	//DY
        	//CameraHolder.instance().keep();
        	//CameraHolder.instance().release();
        	camera = null;
            mPreviewing = false;
        }
    }
    
    private void startPreview() {
    	Log.i("HEBEE", "startPreview");
    	if(mPausing) return;
    	if (mPreviewing) stopPreview();
    	openCamera();
		camera.startPreview();
    	mPreviewing = true;
    }
    
    private void stopPreview() {
    	Log.i("HEBEE", "stopPreview");
    	if (camera != null && mPreviewing) {
            camera.stopPreview();
        }
        mPreviewing = false;
    }

	/* The GLSurfaceView changed
	 * @see android.view.SurfaceHolder.Callback#surfaceChanged(android.view.SurfaceHolder, int, int, int)
	 */
	//@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
	}

	/* The GLSurfaceView was created
	 * The camera will be opened and the preview started 
	 * @see android.view.SurfaceHolder.Callback#surfaceCreated(android.view.SurfaceHolder)
	 */
	//@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Log.i("HEBEE", "surfaceCreated");
		if(!mPreviewing)
			startPreview();  
	}

	/* GLSurfaceView was destroyed
	 * The camera will be closed and the preview stopped.
	 * @see android.view.SurfaceHolder.Callback#surfaceDestroyed(android.view.SurfaceHolder)
	 */
	//@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.i("HEBEE", "surfaceDestroyed");
		
        stopPreview();
        closeCamera();
        
        
      //DY
      cameraHandler.setCam(null);
        
      //DY - closes the camera
    	Message msg = cameraHandler.mConversionHandler.obtainMessage();
		msg.what = cameraHandler.Conversion_Stop;
		cameraHandler.mConversionHandler.sendMessage(msg);
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
    	menu.add(0, CameraPreviewHandler.MODE_RGB, 0, res.getText(R.string.mode_rgb));
	    menu.add(0, CameraPreviewHandler.MODE_GRAY, 0, res.getText(R.string.mode_gray));
	    menu.add(0, CameraPreviewHandler.MODE_BIN, 0, res.getText(R.string.mode_bin));
	    menu.add(0, CameraPreviewHandler.MODE_EDGE, 0, res.getText(R.string.mode_edges));
	    menu.add(0, CameraPreviewHandler.MODE_CONTOUR, 0, res.getText(R.string.mode_contours));   
		return true;
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		this.cameraHandler.setMode(item.getItemId());
		return true;
	}
	
	public void notifyCurrentMarker( int marker )
	{
		Log.i("HEBEE", "notifyCurrentMarker k:" + marker);
		
		boolean bRequestData = false;
		if ( marker > -1 && marker < MAX_NUM_OF_MARKERS )
		{
			if ( lastTime[marker] == null )
	    	{
				Log.i("HEBEE", "Date was null");
				bRequestData = true;
	    	}
	    	else
	    	{
	    		Date now = new Date();
	    		int timeOut = UPDATE_TIMEOUT;
	    		if ( lastCallResult[marker] == false )
	    		{
	    			timeOut = UNSUCCESSFUL_UPDATE_TIMEOUT;
	    		}
	    		//Log.i("HEBEE", "Now "+now.toString()+ " Old " + lastTime[marker].toString() + " Diff msec:" + (now.getTime() - lastTime[marker].getTime()) );
	    		if ( now.getTime() - lastTime[marker].getTime() >= timeOut )
	    		{
	    			Log.i("HEBEE", "Date diff OK! " + timeOut);
	    			bRequestData = true;
	    		}
	    	}
			
			if ( bRequestData )
			{
	    		Message msg = mChildHandler.obtainMessage();
	    		msg.what = ALL_SERVICE;
	    		msg.arg1 = marker;
	            mChildHandler.sendMessage(msg);
	            Log.i("HEBEE", "Sent message to thread1");
	            
	            lastTime[marker] = new Date();
	            lastCallResult[marker] = false;
			}
			
		}
		else
		{
			Log.i("HEBEE", "marker < -1");
		}
		
	}
	
	class RestServiceData
	{
		public int light;
		public float temp;
		public int cpu;
		public int lpm;
		public int cpuAndlpm;
		public int transmit;
		public int listen;
		public int idle_transmit;
		public int idle_listen;
		
//		RestServiceData(
//				int light, float temp, int cpu, int lpm, int cpuAndlpm, int transmit, int listen, int idle_transmit, int idle_listen)
//		{
//			this.light = light;
//			this.temp = temp;
//			this.cpu = cpu;
//			this.lpm = lpm;
//			this.cpuAndlpm = cpuAndlpm;
//			this.transmit = transmit;
//			this.listen = listen;
//			this.idle_transmit = idle_transmit;
//			this.idle_listen = idle_listen;
//		}
	}

}