/******************************************************************************
  Copyright (c) 2015 by Chen.Hu  - 996129302@qq.com
  Copyright (c) 2015 by Tong.zhang(Baida.zhang)  - zt9788@gmail.com
 
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
package com.example.core;

import java.util.ArrayList;
import java.util.List;

import com.example.model.UserInfo;

public class MessageCore {
	
	public static final int MESSAGE_TYPE_TXT = 0xB0;
	public static final int MESSAGE_TYPE_FiLE = 0xB1;
	
	public native void init(String drivceId,String serverip,int serverport,int timeout);
	
	public native void startService();
	
	public native void stopService();
	
	//------------this is block method -------------
	public native UserInfo register(String userName,
			String password/* the password is nothing to do*/);
	
	public native UserInfo login(String name,
			String password/* the password is nothing to do*/);
	
	public native UserInfo addFriend(String friendName);
	
	public native UserInfo findUser(String userName);
	
	public native ArrayList/*<UserInfo>*/ getMyFriends(String username);
	
	public native void delFriend(String userName);
	
	//when say bye it will close socket, you need use start to restart connect
	public native void sayBye();
	//------------------end-----------------------------
	
	public native void sendMessage(String clientMessageid,
							String message,
							int messagetype,
							ArrayList/*<String>*/ sendtosomeone);
	
	//send message,call back method
	public void sendMessageResponse(String messageid,String serverMessageid, int responseCode,
			String error) {
	
	}
	
	/**
	 * call back in recv data from server
	 * @param str
	 */
	public void recvData(String fromDrivceId,String str) {
		
	}
	/**
	 * 
	 * @param fileName
	 */
	public void recvFile(String fromDrivceId,String fileName){
		
	}
	
	
}
