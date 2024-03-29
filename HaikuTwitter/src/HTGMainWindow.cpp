/*
 * Copyright 2010-2012 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include "HTGMainWindow.h"

HTGMainWindow::HTGMainWindow(string key, string secret, int refreshTime, BPoint position, int height)
	: BWindow(BRect(position.x, position.y, position.x+315, position.y+height), "HaikuTwitter", B_DOCUMENT_WINDOW, B_NOT_H_RESIZABLE | B_NOT_ZOOMABLE)
{
	quitOnClose = false; //This is default false
	this->key = key;
	this->secret = secret;
	this->refreshTime = refreshTime;
		
	_retrieveSettings();
	
	this->noAuth = (key.find("NOAUTH") != std::string::npos);
	
	newTweetObj = new twitCurl();
	newTweetObj->setAccessKey( key );
	newTweetObj->setAccessSecret( secret );
	
	/*Get account credentials*/
	accountCredentials = new HTAccountCredentials(newTweetObj, this);
	if(!noAuth)
		accountCredentials->Fetch(); //TODO: This should be done async
	
	/*Set up the menu bar*/
	_SetupMenu();
	
	/*Set up avatar view*/
	fAvatarView = NULL;
	_SetupAvatarView();
	
	/*Set up tab view*/
	BRect tabViewRect(Bounds().left, fAvatarView->Bounds().bottom+19, Bounds().right, Bounds().bottom - 14);
	tabView = new SmartTabView(tabViewRect, "TabView", B_WIDTH_FROM_LABEL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	this->AddChild(tabView);
	
	/*Set up statusbar*/
	statusBar = new HTGStatusBar(Bounds());
	AddChild(statusBar);
	if(accountCredentials->Verified())
		statusBar->SetStatus(accountCredentials->ScreenName());
	else if(noAuth)
		statusBar->SetStatus("Not authenticated");
	else
		statusBar->SetStatus("Authentication failed");
	
	if(!noAuth) {
		/*Set up home timeline*/
		twitCurl *timelineTwitObj = new twitCurl();
		timelineTwitObj->setAccessKey( key );
		timelineTwitObj->setAccessSecret( secret );
		friendsTimeLine = new HTGTimeLineView(timelineTwitObj, TIMELINE_HOME, tabView->Bounds(), "", theSettings.textSize, theSettings.saveTweets);
		tabView->AddTab(friendsTimeLine);
	
		/*Set up mentions timeline*/
		twitCurl *mentionsTwitObj = new twitCurl();
		mentionsTwitObj->setAccessKey( key );
		mentionsTwitObj->setAccessSecret( secret );
		mentionsTimeLine = new HTGTimeLineView(mentionsTwitObj, TIMELINE_MENTIONS, tabView->Bounds(), "", theSettings.textSize, theSettings.saveTweets);
		tabView->AddTab(mentionsTimeLine);
	}
	/*Set up public timeline - if enabled*/
	if(fEnablePublicMenuItem->IsMarked() || noAuth)
		_addPublicTimeLine();
		
	if(noAuth) { //Open a search for #haikuos
		BMessage *newMsg = new BMessage('SRCH');
		newMsg->AddString("text", "#haikuos");
	
		MessageReceived(newMsg);
	}
		
		
	/*Add the saved searches as tabs - if tabs are enabled*/
	if(theSettings.saveSearches && !noAuth)
		_addSavedSearches();
	
	/*Fire a REFRESH message every 'refreshTime' minute*/
	new BMessageRunner(this, new BMessage(REFRESH), refreshTime*1000000*60);
}

//TODO: Fetching saved searches should have it's own class
status_t
addSavedSearchesThreadFunction(void *data) 
{
	BList *args = (BList *)data;
	std::string key = *(std::string *)args->ItemAt(0);
	std::string secret = *(std::string *)args->ItemAt(1);
	SmartTabView *tabView = (SmartTabView *)args->ItemAt(2);
	BRect rect = *(BRect *)args->ItemAt(3);
	int textSize = *(int *)args->ItemAt(4);
	bool saveTweets = *(bool *)args->ItemAt(5);
	
	/*Configure twitter object*/
	twitCurl *twitObj = new twitCurl();
	twitObj->setAccessKey( key );
	twitObj->setAccessSecret( secret );
	
	/*Download saved searches*/
	twitObj->savedSearchGet();
	std::string replyMsg(" ");
	twitObj->getLastWebResponse(replyMsg);
	
	if(replyMsg.length() < 64) {
		#ifdef DEBUG_ENABLED
		std::cout << "Data length to small - Retrying saved searches download..." << std::endl;
		#endif
		
		delete twitObj;
		sleep(5);
		return addSavedSearchesThreadFunction(data);
	}

	
	/*Setup BList to hold the new timeline views*/
	BList *viewList = new BList();
	
	/*Parse saved searches for query*/
	size_t pos = 0;
	size_t i = 0;
	const char *queryTag = "<query>"; 
	while(pos != std::string::npos) {
		pos = replyMsg.find(queryTag, pos);
		if(pos != std::string::npos) {
			int start = pos+strlen(queryTag);
			int end = replyMsg.find("</query>", start);
			std::string searchQuery(replyMsg.substr(start, end-start));
			twitCurl *newTabObj = new twitCurl();
			newTabObj->setAccessKey( key );
			newTabObj->setAccessSecret( secret );
			viewList->AddItem(new HTGTimeLineView(newTabObj, TIMELINE_SEARCH, rect, searchQuery.c_str(), textSize, saveTweets));
			pos = end;
			i++;
		}
	}
	
	/*Parse saved searches for id*/
	pos = 0;
	const char *idTag = "<id>";
	i = 0;
	while(pos != std::string::npos) {
		pos = replyMsg.find(idTag, pos);
		if(pos != std::string::npos) {
			int start = pos+strlen(idTag);
			int end = replyMsg.find("</id>", start);
			std::string searchID(replyMsg.substr(start, end-start));
			HTGTimeLineView *theTimeline = (HTGTimeLineView *)viewList->ItemAt(i);
			if(theTimeline != NULL)
				theTimeline->setSearchID(atoi(searchID.c_str()));
			pos = end;
			i++;
		}
	}
	
	/*Add the timelines to tabView*/
	while(!viewList->IsEmpty()) {
		HTGTimeLineView *theTimeline = (HTGTimeLineView *)viewList->ItemAt(0);
		if(theTimeline != NULL) {
			if(tabView->LockLooper()) {
				tabView->AddTab(theTimeline);
				tabView->UnlockLooper();
			}
		}
		viewList->RemoveItem((int32)0);
	}
	
	/*Clean up*/
	delete twitObj;
	delete viewList;
	delete args;

	return B_OK;
}

void
HTGMainWindow::_addPublicTimeLine()
{
	twitCurl *publicTwitObj = new twitCurl();
	publicTwitObj->setAccessKey( key );
	publicTwitObj->setAccessSecret( secret );
	publicTimeLine = new HTGTimeLineView(publicTwitObj, TIMELINE_PUBLIC, Bounds(), "", theSettings.textSize, theSettings.saveTweets);
	tabView->AddTab(publicTimeLine);	
}

void
HTGMainWindow::_removePublicTimeLine()
{
	for(int i = 2; i < tabView->CountTabs(); i++) {
		if(tabView->TabAt(i)->View() == publicTimeLine)
			tabView->RemoveAndDeleteTab(i);
	}
}

void
HTGMainWindow::_addSavedSearches()
{
	BList *threadArgs = new BList();
	threadArgs->AddItem(&key);
	threadArgs->AddItem(&secret);
	threadArgs->AddItem(tabView);
	threadArgs->AddItem(new BRect(Bounds()));
	threadArgs->AddItem(&theSettings.textSize);
	threadArgs->AddItem(&theSettings.saveTweets);
	
	thread_id theThread = spawn_thread(addSavedSearchesThreadFunction, "UpdateSearches", 10, threadArgs);
	resume_thread(theThread);
}

bool
HTGMainWindow::QuitRequested()
{
	_retrieveSettings();
	_saveSettings();
	if(quitOnClose)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

status_t
HTGMainWindow::_getSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;
	
	path.Append("HaikuTwitter_settings");
	return B_OK;
}

void
HTGMainWindow::_retrieveSettings()
{
	/*Set the defaults, just in case anything bad happens*/
	sprintf(theSettings.username, "changeme");
	sprintf(theSettings.password, "hackme");
	theSettings.refreshTime = 5; //Default refresh time: 5 minutes.
	theSettings.position = BPoint(300, 300);
	theSettings.height = 600;
	theSettings.hideAvatar = false;
	theSettings.enablePublic = false;
	theSettings.saveSearches = false;
	theSettings.saveTweets = true;
	theSettings.textSize = BFont().Size();
	
	BPath path;
	
	if (_getSettingsPath(path) < B_OK)
		return;	
		
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	file.ReadAt(0, &theSettings, sizeof(twitter_settings));
	
	if(theSettings.refreshTime < 0 || theSettings.refreshTime > 10000) {
		std::cout << "Bad refreshtime, reverting to defaults." << std::endl;
		theSettings.refreshTime = 5; //Default refresh time: 5 minutes.
	}
}

status_t
HTGMainWindow::_saveSettings()
{
	theSettings.height = this->Bounds().Height() -23;
	theSettings.position = BPoint(this->Frame().left, this->Frame().top);
	theSettings.hideAvatar = fHideAvatarViewMenuItem->IsMarked();
	theSettings.enablePublic = fEnablePublicMenuItem->IsMarked();
	theSettings.saveTweets = fSaveTweetsMenuItem->IsMarked();
	BFont font;
	tabView->TabAt(0)->View()->GetFont(&font);
	theSettings.textSize = font.Size();
	
	BPath path;
	status_t status = _getSettingsPath(path);
	if (status < B_OK)
		return status;
		
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (status < B_OK)
		return status;
		
	file.WriteAt(0, &theSettings, sizeof(twitter_settings));
	
	return B_OK;
}

void
HTGMainWindow::showAbout()
{
	std::string header("HaikuTwitter unstable");
	std::string text("");
	#ifdef SVN_REV
	text.append("\n\trev. ");
	text.append(SVN_REV);
	#endif
	text.append("\n\tWritten by Martin Hebnes Pedersen\n"
				"\tCopyright 2010-2012\n"
				"\tAll rights reserved.\n"
				"\tDistributed under the terms of the MIT License.\n"
				"\t\n"
				"\tIcon by Michele Frau.\n");
	std::string fulltext = header;
	fulltext.append(text);
	BAlert *alert = new BAlert("about", fulltext.c_str(), "OK");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);
	
	view->GetFont(&font);
	font.SetSize(10);
	view->SetFontAndColor(header.length(), fulltext.length(), &font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, header.length(), &font);

	alert->Go();
}

void
HTGMainWindow::AvatarViewResized()
{
	BRect tabViewRect(Bounds().left, fAvatarView->Bounds().bottom+19, Bounds().right, Bounds().bottom - 14);
		
	tabView->ResizeTo(tabViewRect.Width(), tabViewRect.Height());
	tabView->MoveTo(tabViewRect.left, tabViewRect.top);
}

void
HTGMainWindow::_SetupAvatarView()
{	
	HTTweet* avatar = new HTTweet();
	
	std::string imageUrl("error");
	std::string screenName("---");
	if(accountCredentials->Verified()) {
		imageUrl = std::string(accountCredentials->ProfileImageUrl());
		screenName = std::string(accountCredentials->ScreenName());
	}
	
	avatar->setProfileImageUrl(imageUrl);
	avatar->setScreenName(screenName);
	avatar->downloadBitmap();
	
	BRect viewRect(Bounds().left, fMenuBar->Bounds().bottom, Bounds().right, fMenuBar->Bounds().bottom+1+51);
	if(fHideAvatarViewMenuItem->IsMarked())
		viewRect.bottom = fMenuBar->Bounds().bottom;
	
	fAvatarView = new HTGAvatarView(newTweetObj, this, viewRect);
	fAvatarView->SetAvatarTweet(avatar);
	AddChild(fAvatarView);
}

void
HTGMainWindow::_ResizeAvatarView()
{
	BRect viewRect(Bounds().left, fMenuBar->Bounds().bottom, Bounds().right, fMenuBar->Bounds().bottom+1+51);
	if(fHideAvatarViewMenuItem->IsMarked())
		viewRect.bottom = fMenuBar->Frame().bottom;
	
	fAvatarView->ResizeTo(viewRect.Width(), viewRect.Height());
	
	AvatarViewResized();
}

void
HTGMainWindow::_SetupMenu()
{
	/*Menu bar object*/
	fMenuBar = new BMenuBar(Bounds(), "mbar");
	
	/*Make Twitter Menu*/
	fTwitterMenu = new BMenu("Twitter");
	fTwitterMenu->AddItem(new BMenuItem("New tweet...", new BMessage(NEW_TWEET), 'N'));
	fTwitterMenu->AddSeparatorItem();
	fTwitterMenu->AddItem(new BMenuItem("Go to user...", new BMessage(FIND_USER), 'G'));
	fTwitterMenu->AddItem(new BMenuItem("Search for...", new BMessage(SEARCH_FOR), 'F'));
	fTwitterMenu->AddSeparatorItem();
	fTwitterMenu->AddItem(new BMenuItem("About HaikuTwitter...", new BMessage(ABOUT)));
	fTwitterMenu->AddSeparatorItem();
	fTwitterMenu->AddItem(new BMenuItem("Close active tab", new BMessage(CLOSE_TAB), 'W', B_SHIFT_KEY));
	fTwitterMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	fMenuBar->AddItem(fTwitterMenu);
	
	/*Make View Menu*/
	fViewMenu = new BMenu("View");
	fViewMenu->AddItem(new BMenuItem("Refresh", new BMessage(REFRESH), 'R'));
	
	fViewMenu->AddSeparatorItem();
	
	fHideAvatarViewMenuItem = new BMenuItem("Hide \"What's Happening\"", new BMessage(TOGGLE_AVATARVIEW));
	fHideAvatarViewMenuItem->SetMarked(theSettings.hideAvatar);
	if(!accountCredentials->Verified()) {
		fHideAvatarViewMenuItem->SetMarked(true);
		fHideAvatarViewMenuItem->SetEnabled(false);
	}
	fViewMenu->AddItem(fHideAvatarViewMenuItem);
	
	fEnablePublicMenuItem = new BMenuItem("Show public stream", new BMessage(TOGGLE_PUBLIC));
	fViewMenu->AddItem(fEnablePublicMenuItem);
	fEnablePublicMenuItem->SetMarked(theSettings.enablePublic);
	
	fViewMenu->AddSeparatorItem();

	BMenu *textSizeSubMenu = new BMenu("Text Size");
	textSizeSubMenu->AddItem(new BMenuItem("Increase", new BMessage(TEXT_SIZE_INCREASE), '+'));
	textSizeSubMenu->AddItem(new BMenuItem("Decrease", new BMessage(TEXT_SIZE_DECREASE), '-'));
	textSizeSubMenu->AddItem(new BMenuItem("Revert", new BMessage(TEXT_SIZE_REVERT)));
	fViewMenu->AddItem(textSizeSubMenu);
	
	fMenuBar->AddItem(fViewMenu);
	
	/*Make Settings Menu*/
	fSettingsMenu = new BMenu("Settings");
	fMenuBar->AddItem(fSettingsMenu);
	fAutoStartMenuItem = new BMenuItem("Auto start at login", new BMessage(TOGGLE_AUTOSTART));
	fAutoStartMenuItem->SetMarked(_isAutoStarted());
	fSettingsMenu->AddItem(fAutoStartMenuItem);
	fSaveTweetsMenuItem = new BMenuItem("Download tweets to filesystem", new BMessage(TOGGLE_SAVETWEETS));
	fSaveTweetsMenuItem->SetMarked(theSettings.saveTweets);
	fSettingsMenu->AddItem(fSaveTweetsMenuItem);
	
	fSettingsMenu->AddSeparatorItem();
	fSettingsMenu->AddItem(new BMenuItem("Notifications...", new BMessage(INFOPOPPER_SETTINGS)));
	fSettingsMenu->AddItem(new BMenuItem("Preferences...", new BMessage(ACCOUNT_SETTINGS)));
	
	AddChild(fMenuBar);
}

/*This function checks for a file named HaikuTwitter in the users launch-folder, 
 * it does not check if it's a valid symlink or even an executable.
 */
bool
HTGMainWindow::_isAutoStarted()
{
	BPath path;
	status_t status = find_directory(B_USER_CONFIG_DIRECTORY, &path);
	if (status < B_OK) {
		HTGErrorHandling::displayError("Unable to locate user's config directory.");
		return false;
	}
	
	path.Append("boot/launch/HaikuTwitter");
	
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return false;
	else
		return true;
}

void
HTGMainWindow::_setAutoStarted(bool autostarted)
{
	BPath launchPath;
	BPath installPath;
	
	status_t status = find_directory(B_USER_CONFIG_DIRECTORY, &launchPath);
	if (status < B_OK) {
		HTGErrorHandling::displayError("Unable to locate user's config directory.");
		return;
	}
	status = find_directory(B_APPS_DIRECTORY, &installPath);
	if (status < B_OK) {
		HTGErrorHandling::displayError("Unable to locate the default applications directory.");
		return;
	}
	
	launchPath.Append("boot/launch/");
	installPath.Append("HaikuTwitter/");
	
	BDirectory launchDir(launchPath.Path());
	BDirectory installDir(installPath.Path());
	
	if (launchDir.Contains("HaikuTwitter", B_SYMLINK_NODE) && !autostarted) {//Delete symlink
		BEntry *entry = new BEntry();
		launchDir.FindEntry("HaikuTwitter", entry, false);
		if(entry->Remove() < B_OK)
			HTGErrorHandling::displayError("Unable to delete symbolic link.");
		delete entry;
	}
	
	if (!installDir.Contains("HaikuTwitter", B_FILE_NODE)) {//HaikuTwitter not installed in default location
		std::string errorMsg("Executable not found in default location (");
		errorMsg.append(installPath.Path());
		errorMsg.append(").\n\nCould not create symbolic link.");
		HTGErrorHandling::displayError(errorMsg.c_str());
	}
	
	if (!launchDir.Contains("HaikuTwitter", B_SYMLINK_NODE) && autostarted) {//Create symlink
		installPath.Append("HaikuTwitter");
		if(launchDir.CreateSymLink("HaikuTwitter", installPath.Path(), NULL) < B_OK)
			HTGErrorHandling::displayError("Unable to create symbolic link.");
	}
}

void
HTGMainWindow::MessageReceived(BMessage *msg)
{
	const char* text_label = "text";
	const char* id_label = "reply_to_id";
	switch(msg->what) {
		case STATUS_UPDATED:
			{
				HTGTimeLineView *homeTimeline = dynamic_cast<HTGTimeLineView*>(tabView->TabAt(0)->View());
				homeTimeline->updateTimeLine();
			}
			break;
		case POST:
			fAvatarView->MessageReceived(msg);
			break;
		case TOGGLE_AUTOSTART:
			_setAutoStarted(!fAutoStartMenuItem->IsMarked());
			fAutoStartMenuItem->SetMarked(_isAutoStarted());
			break;
		case TOGGLE_AVATARVIEW:
			fHideAvatarViewMenuItem->SetMarked(!fHideAvatarViewMenuItem->IsMarked());
			theSettings.hideAvatar = fHideAvatarViewMenuItem->IsMarked();
			_ResizeAvatarView();
			break;
		case TOGGLE_SAVETWEETS:
			fSaveTweetsMenuItem->SetMarked(!fSaveTweetsMenuItem->IsMarked());
			theSettings.saveTweets = fSaveTweetsMenuItem->IsMarked();
			for(int i = 0; i < tabView->CountTabs(); i++) {
				HTGTimeLineView *current = dynamic_cast<HTGTimeLineView*>(tabView->TabAt(i)->View());
				if(current != NULL)
					current->setSaveTweets(fSaveTweetsMenuItem->IsMarked());
			}
			break;
		case TOGGLE_PUBLIC:
			fEnablePublicMenuItem->SetMarked(!fEnablePublicMenuItem->IsMarked());
			theSettings.enablePublic = fEnablePublicMenuItem->IsMarked();
			if(fEnablePublicMenuItem->IsMarked())
				_addPublicTimeLine();
			else
				_removePublicTimeLine();
			break;
		case NEW_TWEET:
			newTweetWindow = new HTGNewTweetWindow(newTweetObj, this);
			newTweetWindow->SetText(msg->FindString(text_label, (int32)0)); //Set text (RT, reply, ie)
			newTweetWindow->setTweetId(msg->FindString(id_label, (int32)0)); //Set id (RT, reply, ie)
			newTweetWindow->Show();
			break;
		case NEW_RETWEET:
			{
				std::string id(msg->FindString("retweet_id"));
				newTweetObj->statusRetweet(id);
				HTGTimeLineView *homeTimeline = dynamic_cast<HTGTimeLineView*>(tabView->TabAt(0)->View());
				homeTimeline->updateTimeLine();
			}
		case REFRESH:
			for(int i = 0; i < tabView->CountTabs(); i++) {
				HTGTimeLineView *current = dynamic_cast<HTGTimeLineView*>(tabView->TabAt(i)->View());
				if(current != NULL)
					current->updateTimeLine();
			}
			break;
		case FIND_USER:
			goToUserWindow = new HTGGoToUserWindow(this);
			goToUserWindow->Show();
			break;
		case SEARCH_FOR:
			searchForWindow = new HTGSearchForWindow(this);
			searchForWindow->Show();
			break;
		case GO_USER:
			if(false) {
				timeLineWindow = new HTGTimeLineWindow(this, key, secret, refreshTime, TIMELINE_USER, msg->FindString(text_label, (int32)0));
				timeLineWindow->Show();
			}
			else if (LockLooper()) {
				newTabObj = new twitCurl();
				newTabObj->setAccessKey( key );
				newTabObj->setAccessSecret( secret );
				tabView->AddTab(new HTGTimeLineView(newTabObj, TIMELINE_USER, Bounds(), msg->FindString(text_label, (int32)0), theSettings.textSize, true), true);
				tabView->Select(tabView->CountTabs()-1); //Select the new tab
				UnlockLooper();
			}
			break;
		case GO_SEARCH:
			if(false) {
				timeLineWindow = new HTGTimeLineWindow(this, key, secret, refreshTime, TIMELINE_SEARCH, msg->FindString(text_label, (int32)0));
				timeLineWindow->Show();
			}
			else if (LockLooper()) {
				bool excists = false;
				for(int i = 0; i < tabView->CountTabs(); i++)
					 if(strcmp(tabView->TabAt(i)->Label(), msg->FindString(text_label, (int32)0)) == 0) { //Parse for excisting tab
					 	excists = true;
					 	tabView->Select(i);
					 	break;
					 }
				if(!excists) {
					newTabObj = new twitCurl();
					newTabObj->setAccessKey( key );
					newTabObj->setAccessSecret( secret );
					HTGTimeLineView *newTimeline = new HTGTimeLineView(newTabObj, TIMELINE_SEARCH, Bounds(), msg->FindString(text_label, (int32)0), theSettings.textSize, true);
					tabView->AddTab(newTimeline, true); //Add the new timeline
					if(theSettings.saveSearches)
						newTimeline->savedSearchCreateSelf(); //Save the search on twitter
					tabView->Select(tabView->CountTabs()-1); //Select the new tab
				}
				UnlockLooper();
			}
			break;
		case CLOSE_TAB:
			if(tabView->Selection() >= 2) //Don't close hardcoded tabs
				tabView->RemoveAndDeleteTab(tabView->Selection());
			break;
		case ACCOUNT_SETTINGS:
			accountSettingsWindow = new HTGAccountSettingsWindow(this);
			accountSettingsWindow->Show();
			break;
		case INFOPOPPER_SETTINGS:
			infopopperSettingsWindow = new HTGInfoPopperSettingsWindow();
			infopopperSettingsWindow->Show();
			break;
		case ABOUT:
			showAbout();
			break;
		case TEXT_SIZE_INCREASE:
		case TEXT_SIZE_DECREASE:
		case TEXT_SIZE_REVERT:
		{
			BFont font;
			tabView->TabAt(0)->View()->GetFont(&font);
			size_t size = font.Size();
			if(msg->what == TEXT_SIZE_INCREASE)
				size += 1;
			else if(msg->what == TEXT_SIZE_DECREASE)
				size -= 1;
			else
				size = BFont().Size(); //BE_PLAIN_FONT used
				
			//Limit the font size
			if (size < 9)
				size = 9;
			if (size > 18)
				size = 18;
			
			_setTimelineTextSize(size);
			theSettings.textSize = size;
			break;
		}
		case B_CLOSE_REQUESTED:
			if(quitOnClose)
				be_app->PostMessage(B_QUIT_REQUESTED);
			else
				BWindow::MessageReceived(msg);
			break;
		default:
			be_app->MessageReceived(msg);
			BWindow::MessageReceived(msg);
	}
}

void
HTGMainWindow::setQuitOnClose(bool quitOnClose)
{
	this->quitOnClose = quitOnClose;
}

void
HTGMainWindow::_setTimelineTextSize(int size)
{
	BFont font;
	tabView->TabAt(0)->View()->GetFont(&font);
	font.SetSize(size);
	
	for(int i = 0; i < tabView->CountTabs(); i++) {
		HTGTimeLineView *current = dynamic_cast<HTGTimeLineView*>(tabView->TabAt(i)->View());
		if(current != NULL)
			current->SetFont(&font);
	}
}

HTGMainWindow::~HTGMainWindow()
{
	//This should really not be empty anymore!!
}
