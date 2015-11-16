package com.example.test;

import com.nostra13.universalimageloader.cache.disc.naming.Md5FileNameGenerator;
import com.nostra13.universalimageloader.cache.memory.impl.UsingFreqLimitedMemoryCache;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.QueueProcessingType;

import android.app.Application;
import android.content.Context;

public class MyApplication extends Application {

	public static Context context;
	public static ImageLoader mImageLoader = ImageLoader.getInstance();

	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();
		context = this;
		try {
			super.onCreate();
			ImageLoaderConfiguration config = new ImageLoaderConfiguration.Builder(
					getApplicationContext())
					.denyCacheImageMultipleSizesInMemory()
					.memoryCacheExtraOptions(768, 1280)
					.memoryCache(
							new UsingFreqLimitedMemoryCache(5 * 1024 * 1024))
					.memoryCacheSize(5 * 1024 * 1024)
					.discCacheSize(50 * 1024 * 1024)
					.discCacheFileNameGenerator(new Md5FileNameGenerator())
					.threadPoolSize(5).threadPriority(Thread.NORM_PRIORITY - 1)
					.tasksProcessingOrder(QueueProcessingType.LIFO).build();
			// 初始化ImageLoader的与配置。
			mImageLoader.init(config);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

}
