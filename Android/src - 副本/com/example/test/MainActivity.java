package com.example.test;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
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

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		registerMessageReceiver();
		final EditText edit = (EditText) findViewById(R.id.edit);
		listView = (ListView) findViewById(R.id.list);
		Button send = (Button) findViewById(R.id.send);
		final ArrayList list = new ArrayList();
		imList = new ArrayList<IMBean>();
		adapter = new IMAdapter(getApplicationContext(), imList);
		listView.setAdapter(adapter);
		// list.get(index)
		list.add("kuer");
		sc.start("223.202.67.123", 1234, "test");
		send.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				sc.sendMessageNative("test",0xF11,edit.getText().toString(), list);
				IMBean im = new IMBean(edit.getText().toString(), 2);
				imList.add(im);
				adapter.notifyDataSetChanged();
				edit.setText("");
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
			IMBean im = new IMBean(intent.getStringExtra("str"), 1);
			imList.add(im);
			adapter.notifyDataSetChanged();
		}
	}

}
