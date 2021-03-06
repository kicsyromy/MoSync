/* Copyright 2013 David Axmark

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * @file MainScreen.cpp
 * @author Emma Tresanszki and Bogdan Iusco
 *
 * @brief Application's main screen(tab screen).
 */

#include "MainScreen.h"
#include "SettingsScreen.h"
#include "DisplayNotificationScreen.h"
#include "TCPConnection.h"
#include "HTTPConnection.h"
#include "Util.h"
#include "Config.h"

/**
 * Constructor.
 */
MainScreen::MainScreen() :
	TabScreen(),
	mDisplayNotificationScreen(NULL),
	mSettingsScreen(NULL),
	mTCPConnection(NULL),
	mToken(NULL)
{
	mDisplayNotificationScreen = new DisplayNotificationScreen();
	mSettingsScreen = new SettingsScreen(this);
	if ( isAndroid() )
	{
		mHttpConnection = new HTTPConnection(this);
	}
	else if ( isIOS() )
	{
		mTCPConnection = new TCPConnection(this);
	}

	this->addTab(mDisplayNotificationScreen);
	this->addTab(mSettingsScreen);

	if ( isAndroid() )
	{
		// Store the reg ID, and call registration only once per app.
		checkStore();
	}
	else
	{
		int registerCode = Notification::NotificationManager::getInstance()->registerPushNotification(
				Notification::PUSH_NOTIFICATION_TYPE_BADGE |
				Notification::PUSH_NOTIFICATION_TYPE_ALERT |
				Notification::PUSH_NOTIFICATION_TYPE_SOUND,
				GCM_PROJECT_ID);

		if ( MA_NOTIFICATION_RES_UNSUPPORTED == registerCode )
			maPanic(0, "This device does not support push notifications");
	}

	NotificationManager::getInstance()->addPushNotificationListener(this);
}

/**
 * Used for Android only.
 * Check if the store exists.
 * If it does not exist, call the registration method,
 * create the store for later writing to it after the
 * connection to the server is established.
 */
void MainScreen::checkStore() {
	MAHandle myStore = maOpenStore("MyPushStore", 0);

	if (myStore == STERR_NONEXISTENT)
	{
		mSendRegistrationNeeded = true;

		// Request registration ID.
		int registerCode = Notification::NotificationManager::getInstance()->registerPushNotification(
				Notification::PUSH_NOTIFICATION_TYPE_BADGE
						| Notification::PUSH_NOTIFICATION_TYPE_ALERT
						| Notification::PUSH_NOTIFICATION_TYPE_SOUND,
				GCM_PROJECT_ID);
		if ( MA_NOTIFICATION_RES_UNSUPPORTED == registerCode )
			maPanic(0, "This device does not support push notifications");

		// Close store without deleting it.
		maCloseStore(myStore, 0);
	}
	else
	{
		mDisplayNotificationScreen->pushRegistrationAlreadyCompleted();
		mSendRegistrationNeeded = false;
		// Close store without deleting it.
		maCloseStore(myStore, 0);
	}
}

/**
 * Used for Android only.
 * Stores the registration ID in a store for later use.
 * @param token The registration_ID.
 */
void MainScreen::storeRegistrationID(MAUtil::String* token)
{
	mToken = token;

	// Store doesn't exist.
	MAHandle myStore = maOpenStore("MyPushStore", MAS_CREATE_IF_NECESSARY);

	// Create store and write Registration ID
	MAHandle myData = maCreatePlaceholder();
	if(maCreateData(myData, token->length()) == RES_OK)
	{
		 maWriteData(myData, token->c_str(), 0, token->length());
		 // Write the data resource to the store.
		 int result = maWriteStore(myStore, myData);

		 if ( result < 0 )
		 {
			 printf("Cannot write to store the token!!");
		 }
		 maCloseStore(myStore, 0);
	}

}

/**
 * Destructor.
 */
MainScreen::~MainScreen()
{
	delete mToken;
	if ( mTCPConnection )
		delete mTCPConnection;
	NotificationManager::getInstance()->removePushNotificationListener(this);
}

/**
 * Called when the application receives a push notification.
 * @param pushNotification The received push notification.
 */
void MainScreen::didReceivePushNotification(
    PushNotification& pushNotification)
{
	printf("MainScreen::didReceivePushNotification");
	mDisplayNotificationScreen->pushNotificationReceived(pushNotification);
}


/**
 * Called when application has been registered for push notifications.
 */
void MainScreen::didApplicationRegistered(MAUtil::String& token)
{
	printf("MainScreen::didApplicationRegistered");
	printf(token.c_str());
	mToken = new MAUtil::String(token);
	mDisplayNotificationScreen->pushRegistrationDone(true);
    //mMessageLabel->setText("Your app registered for push notifications");
    // Now you can press Connect to connect and send data to the server.
}

/**
 * Called when the application has been unregistered for push notifications.
 * Platform: Android only.
 */
void MainScreen::didApplicationUnregister()
{
	printf("MainScreen::didApplicationUnregister");
}

/**
 * Called if the application did not registered for push notifications.
 */
void MainScreen::didFailedToRegister(
    MAUtil::String& error)
{
	printf("MainScreen::didFailedToRegister");
	mDisplayNotificationScreen->pushRegistrationDone(false);
}

/**
 * Called when the application is connected to the server.
 */
void MainScreen::connectionEstablished()
 {
	printf("MainScreen::connectionEstablished()");
	// Update the UI.
	mSettingsScreen->connectionEstablished();

	// Finally send it over TCP to the iOS server,
	// or over HTTP to the Java web app.
	if (isIOS() && mToken)
	{
		mTCPConnection->sendData(*mToken);
	}
}

/**
 * Called when the server has received the authorization key.
 */
void MainScreen::messageSent()
{
	connectionEstablished();
	// This is the first time the app is launched on Android phone.
	// Need to send the token. Also, store it to the store.
	if (isAndroid() && mToken)
	{
		storeRegistrationID(mToken);
	}
}

/**
 * Called when connect button is pressed.
 * @param ipAddress Server's ip address written by the user.
 * @param port Server's port written by the user.
 */
void MainScreen::connectToServer(const MAUtil::String& ipAddress,
		const MAUtil::String& port)
{
	int resultCode;
	if ( isAndroid() )
	{
		resultCode = mHttpConnection->sendApiKey(ipAddress, *mToken);
	}
	else if( isIOS() )
	{
		resultCode = mTCPConnection->connect(ipAddress, port);
	}
	if (resultCode <= 0 )
	{
		mSettingsScreen->connectionFailed();
	}
}
