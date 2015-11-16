package com.example.test;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class IMAdapter extends BaseAdapter {

	private Context context;
	private List<IMBean> list;

	public IMAdapter(Context context, List<IMBean> list) {
		super();
		this.context = context;
		this.list = list;
	}

	@Override
	public int getCount() {
		// TODO Auto-generated method stub
		return list.size();
	}

	@Override
	public Object getItem(int position) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public long getItemId(int position) {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		// TODO Auto-generated method stub
		ViewHolder vh;
		if (convertView == null) {
			convertView = LayoutInflater.from(context).inflate(R.layout.item,
					null);
			vh = new ViewHolder();
			vh.type1 = (TextView) convertView.findViewById(R.id.type1);
			vh.type2 = (TextView) convertView.findViewById(R.id.type2);
			convertView.setTag(vh);
		} else {
			vh = (ViewHolder) convertView.getTag();
		}

		if (list.get(position).getType() == 1) {
			vh.type2.setVisibility(View.GONE);
			vh.type1.setText(list.get(position).getMessage());
		} else {
			vh.type1.setVisibility(View.GONE);
			vh.type2.setText(list.get(position).getMessage());
		}

		return convertView;
	}

	class ViewHolder {
		TextView type1, type2;
	}

}
