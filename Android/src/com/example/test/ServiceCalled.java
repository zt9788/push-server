package com.example.test;

import java.util.List;

import android.content.Intent;
import android.util.Log;

public class ServiceCalled {

	public void recvData(String str) {
		// TODO 把str添加到，文本框中。这个方法不用管，自动调用的
		// 如果在，消息栏里面有提示就更好了
		Log.d("test", str);
		Intent intent = new Intent(MainActivity.MESSAGE_RECEIVED_ACTION);
		intent.putExtra("str", str);
		intent.putExtra("message_type", Cache.MESSAGE_TYPE_TEXT);
		MyApplication.context.sendBroadcast(intent);
	}

	public void recvFile(String fileName) {
		// TODO 接收的文件
		Log.d("test", fileName);
		Intent intent = new Intent(MainActivity.MESSAGE_RECEIVED_ACTION);
		intent.putExtra("str", "file:/" + fileName);
		intent.putExtra("message_type", Cache.MESSAGE_TYPE_FILE);
		MyApplication.context.sendBroadcast(intent);
	}

	// 服务启动的方法里面
	public void startService() {
		// 启动服务，服务里面启动一个方法
		// start();
	}

	/**
	 * 
	 * @param driveceid
	 *            设备id
	 * @param message
	 *            发送内容
	 * @param sendtosomeone
	 *            发送给谁
	 */
	public boolean sendMessage(String driveceid, String message,
			List<String> sendtosomeone) {
		return false;
		// 点按钮发送消息
		// sendMessageNative
	}
	//call back method
	public void sendMessageResponse(String messageid, int responseCode,
			String error) {
		Log.d("test", messageid+"-"+responseCode);		
		// TODO 发送成功
		if (responseCode == Cache.SEND_SUCCESS) {
			Log.d("test", messageid+"-"+responseCode);	
			Intent intent = new Intent(MainActivity.MESSAGE_RECEIVED_ACTION);
			intent.putExtra("messageid", messageid);
			MyApplication.context.sendBroadcast(intent);
		}
	}

	// public static native void init();

	public native void start(String serverIP, int port, String myDriveid);

	public native void stop();

	public native boolean sendMessageNative(String driveceid,
												String messageid,
																int type/*
																	 * 0xF11=txt,
																	 * 0xF22
																	 * =file
																	 */,
													String message, 
													List<String> sendtosomeone);

}
