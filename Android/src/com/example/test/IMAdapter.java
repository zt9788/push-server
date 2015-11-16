package com.example.test;

import java.util.List;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class IMAdapter extends BaseAdapter {

	private Context context;
	private List<IMBean> list;
	private ImageLoader imageLoder;
	private DisplayImageOptions dio;

	public IMAdapter(Context context, List<IMBean> list) {
		super();
		this.context = context;
		this.list = list;
		imageLoder = ImageLoader.getInstance();
		dio = ImageDisplayOptions.imageOptions(R.drawable.ic_launcher);
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
			vh.image1 = (ImageView) convertView.findViewById(R.id.image1);
			vh.image2 = (ImageView) convertView.findViewById(R.id.image2);
			vh.linear_text = (RelativeLayout) convertView
					.findViewById(R.id.linear_text);
			vh.linear_image = (RelativeLayout) convertView
					.findViewById(R.id.linear_image);
			vh.linear_text_p = (ProgressBar) convertView
					.findViewById(R.id.linear_text_p);
			vh.linear_image_p = (ProgressBar) convertView
					.findViewById(R.id.linear_image_p);
			convertView.setTag(vh);
		} else {
			vh = (ViewHolder) convertView.getTag();
			resetView(vh);
		}
		if (list.get(position).getMessage_type() == Cache.MESSAGE_TYPE_TEXT) {
			vh.image1.setVisibility(View.GONE);
			vh.image2.setVisibility(View.GONE);
			if (list.get(position).getType() == Cache.SEND_FROM) {
				vh.linear_text.setVisibility(View.GONE);
				vh.type1.setText(list.get(position).getMessage());
			} else {
				if (list.get(position).getStatus() == Cache.SEND_ING) {
					vh.linear_text_p.setVisibility(View.VISIBLE);
				} else {
					vh.linear_text_p.setVisibility(View.GONE);
				}
				vh.type1.setVisibility(View.GONE);
				vh.type2.setText(list.get(position).getMessage());
			}
		} else {
			vh.type1.setVisibility(View.GONE);
			vh.type2.setVisibility(View.GONE);
			if (list.get(position).getType() == Cache.SEND_FROM) {
				vh.linear_text.setVisibility(View.GONE);
				imageLoder.displayImage(list.get(position).getMessage(),
						vh.image1, dio);
			} else {
				vh.image1.setVisibility(View.GONE);
				if (list.get(position).getStatus() == Cache.SEND_ING) {
					vh.linear_image_p.setVisibility(View.VISIBLE);
				} else {
					vh.linear_image_p.setVisibility(View.GONE);
				}
				imageLoder.displayImage(list.get(position).getMessage(),
						vh.image2, dio);

			}
		}

		return convertView;
	}

	class ViewHolder {
		RelativeLayout linear_text, linear_image;
		ProgressBar linear_text_p, linear_image_p;
		TextView type1, type2;
		ImageView image1, image2;
	}

	public void resetView(ViewHolder vh) {
		vh.linear_text.setVisibility(View.VISIBLE);
		vh.type2.setVisibility(View.VISIBLE);
	}

}
