package com.example.test;

public class IMBean {

	private String message;
	private int type;
	private int message_type;
	private int status;
	private String uuId;

	public IMBean() {
		super();
	}

	public IMBean(String message, int type, int message_type, int status,
			String uuId) {
		super();
		this.message = message;
		this.type = type;
		this.message_type = message_type;
		this.status = status;
		this.uuId = uuId;
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

	public int getMessage_type() {
		return message_type;
	}

	public void setMessage_type(int message_type) {
		this.message_type = message_type;
	}

	public int getStatus() {
		return status;
	}

	public void setStatus(int status) {
		this.status = status;
	}

	public String getUuId() {
		return uuId;
	}

	public void setUuId(String uuId) {
		this.uuId = uuId;
	}

	@Override
	public String toString() {
		return "IMBean [message=" + message + ", type=" + type
				+ ", message_type=" + message_type + ", status=" + status
				+ ", uuId=" + uuId + "]";
	}

}
