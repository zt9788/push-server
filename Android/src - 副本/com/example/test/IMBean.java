package com.example.test;

public class IMBean {

	private String message;
	private int type;

	public IMBean() {
		super();
	}

	public IMBean(String message, int type) {
		super();
		this.message = message;
		this.type = type;
	}

	public String getMessage() {
		return message;
	}

	public void setMessage(String message) {
		this.message = message;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	@Override
	public String toString() {
		return "IMBean [message=" + message + ", type=" + type + "]";
	}

}
