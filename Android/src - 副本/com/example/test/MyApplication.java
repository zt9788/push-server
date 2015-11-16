package com.example.test;

import android.app.Application;
import android.content.Context;

public class MyApplication extends Application {

	public static Context context;

	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();
		context = this;
	}

}
