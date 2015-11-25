#!/bin/sh
javah -jni -o preview_handler_jni.h -classpath ../bin/ edu.dhbw.andopenglcam.CameraPreviewHandler
javah -jni -o marker_info.h -classpath ../bin/ edu.dhbw.andopenglcam.MarkerInfo
