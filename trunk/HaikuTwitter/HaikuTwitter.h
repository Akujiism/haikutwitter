/*
 * Copyright 2010 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include <iostream>
#include <cstdlib>
#include <string.h>

#include <Application.h>

#include "twitcurl/twitcurl.h"

#include "HTStorage.h"
#include "HTGMainWindow.h"
#include "HTGAuthorizeWindow.h"
#include "HTGTweetViewWindow.h"

#ifndef HAIKUTWITTER
#define HAIKUTWITTER

status_t getSettingsPath(BPath &path);
struct twitter_settings retrieveSettings();
struct oauth_settings retrieveOAuth();

class HaikuTwitter : public BApplication {
public:
								HaikuTwitter();
	virtual						~HaikuTwitter();
	
	virtual	void				RefsReceived(BMessage* message);
	virtual void				MessageReceived(BMessage *message);
private:
			HTGMainWindow*		mainWindow;
			HTGTweetViewWindow*	tweetViewWindow;	
};
#endif
