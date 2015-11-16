package com.example.test;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.UUID;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Toast;

public class MainActivity extends Activity {

	static {
		System.loadLibrary("Test");
	}
	ServiceCalled sc = new ServiceCalled();
	private ListView listView;
	private List<IMBean> imList;
	private IMAdapter adapter;
	private Button pic;
	final ArrayList list = new ArrayList();
	private String mine = "test";

	public static String getUUID() {
		String uuid = UUID.randomUUID().toString().trim().replaceAll("-", "");
		return uuid;
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		registerMessageReceiver();
		final EditText edit = (EditText) findViewById(R.id.edit);
		listView = (ListView) findViewById(R.id.list);
		Button send = (Button) findViewById(R.id.send);
		pic = (Button) findViewById(R.id.pic);
		imList = new ArrayList<IMBean>();
		adapter = new IMAdapter(getApplicationContext(), imList);
		listView.setAdapter(adapter);
		// list.get(index)
		list.add("abc");
		list.add("kuer");
		sc.start("223.202.67.123", 1234, mine);
		send.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				String messageid = getUUID();
				sc.sendMessageNative(mine, messageid,Cache.MESSAGE_TYPE_TEXT, edit
						.getText().toString(), list);
				IMBean im = new IMBean(edit.getText().toString(),
						Cache.SEND_TO, Cache.MESSAGE_TYPE_TEXT, Cache.SEND_ING,
						messageid);
				imList.add(im);
				adapter.notifyDataSetChanged();
				edit.setText("");
				listView.setSelection(imList.size());
			}
		});
		pic.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				Intent intent = new Intent(
						Intent.ACTION_PICK,
						android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
				startActivityForResult(intent, 400);
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		unregisterReceiver(mMessageReceiver);
	}

	private MessageReceiver mMessageReceiver;
	public static final String MESSAGE_RECEIVED_ACTION = "recvData";

	public void registerMessageReceiver() {
		mMessageReceiver = new MessageReceiver();
		IntentFilter filter = new IntentFilter();
		filter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY);
		filter.addAction(MESSAGE_RECEIVED_ACTION);
		registerReceiver(mMessageReceiver, filter);
	}

	public class MessageReceiver extends BroadcastReceiver {

		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent.getStringExtra("messageid") != null) {
				for (int i = 0; i < imList.size(); i++) {
					if (imList.get(i).getUuId()
							.equals(intent.getStringExtra("messageid"))) {
						imList.get(i).setStatus(Cache.SEND_SUCCESS);
						adapter.notifyDataSetChanged();
						return;
					}
				}
			} else {
				if (intent.getIntExtra("message_type", 0) == Cache.MESSAGE_TYPE_TEXT) {
					IMBean im = new IMBean(intent.getStringExtra("str"),
							Cache.SEND_FROM, Cache.MESSAGE_TYPE_TEXT,
							Cache.SEND_SUCCESS, "");
					imList.add(im);
					adapter.notifyDataSetChanged();
				} else if (intent.getIntExtra("message_type", 0) == Cache.MESSAGE_TYPE_FILE) {
					IMBean im = new IMBean(intent.getStringExtra("str"),
							Cache.SEND_FROM, Cache.MESSAGE_TYPE_FILE,
							Cache.SEND_SUCCESS, "");
					imList.add(im);
					adapter.notifyDataSetChanged();
				}
				listView.setSelection(imList.size());
			}
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		// TODO Auto-generated method stub
		super.onActivityResult(requestCode, resultCode, data);
		if (requestCode == 400) {
			if (resultCode == Activity.RESULT_OK) {
				Uri uri = data.getData();
				String[] proj = { MediaStore.Images.Media.DATA };
				Cursor actualimagecursor = managedQuery(uri, proj, null, null,
						null);
				int actual_image_column_index = actualimagecursor
						.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
				actualimagecursor.moveToFirst();
				String img_path = actualimagecursor
						.getString(actual_image_column_index);
				System.out.println("图片真实路径：" + img_path);
				String messageid = getUUID();
				sc.sendMessageNative(mine,messageid ,Cache.MESSAGE_TYPE_FILE, img_path,
						list);
				IMBean im = new IMBean("file:/" + img_path, Cache.SEND_TO,
						Cache.MESSAGE_TYPE_FILE, Cache.SEND_ING, messageid);
				imList.add(im);
				adapter.notifyDataSetChanged();
				listView.setSelection(imList.size());
			}
		}
	}

}
