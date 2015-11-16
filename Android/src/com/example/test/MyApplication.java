/******************************************************************************
  Copyright (c) 2015 by Chen.Hu  - 996129302@qq.com
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
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
