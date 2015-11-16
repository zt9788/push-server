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
