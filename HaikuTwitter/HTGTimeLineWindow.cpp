/*
 * Copyright 2010 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include "HTGTimeLineWindow.h"

HTGTimeLineWindow::HTGTimeLineWindow(twitCurl *twitObj) : BWindow(BRect(300, 300, 615, 900), "HaikuTwitter", B_TITLED_WINDOW, 0) {	
	this->twitObj = twitObj;
	
	/*Set up the menu bar*/
	_SetupMenu();
	
	/*Set up tab view*/
	tabView = new BTabView(BRect(0, 20, 315, 600), "TabView");
	this->AddChild(tabView);
	
	/*Set up timeline*/
	friendsTimeLine = new HTGTimeLineView(twitObj, TIMELINE_FRIENDS);
	tabView->AddTab(friendsTimeLine);
	
	/*Set up mentions timeline*/
	mentionsTimeLine = new HTGTimeLineView(twitObj, TIMELINE_MENTIONS);
	tabView->AddTab(mentionsTimeLine);
	
	/*Set up public timeline - Weird symbols on the public timeline, probably a language with non UTF-8 symbols*/
	//tabView->AddTab(new HTGTimeLineView(twitObj, TIMELINE_PUBLIC));
}

bool HTGTimeLineWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void HTGTimeLineWindow::_SetupMenu() {
	/*Menu bar object*/
	fMenuBar = new BMenuBar(Bounds(), "mbar");
	
	/*Make Twitter Menu*/
	fTwitterMenu = new BMenu("Twitter");
	fTwitterMenu->AddItem(new BMenuItem("New tweet", new BMessage(NEW_TWEET), 'N'));
	fTwitterMenu->AddItem(new BMenuItem("Refresh", new BMessage(REFRESH), 'R'));
	fTwitterMenu->AddSeparatorItem();
	fTwitterMenu->AddItem(new BMenuItem("About HaikuTwitter...", new BMessage(ABOUT)));
	fTwitterMenu->AddSeparatorItem();
	fTwitterMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	fMenuBar->AddItem(fTwitterMenu);
	
	/*Make Edit Menu*/
	fEditMenu = new BMenu("Edit");
	fEditMenu->AddItem(new BMenuItem("Copy", new BMessage(B_COPY), 'C')); //This is not implemented yet.
	fMenuBar->AddItem(fEditMenu);
	
	/*Make Settings Menu*/
	fSettingsMenu = new BMenu("Settings");
	fSettingsMenu->AddItem(new BMenuItem("Account...", new BMessage(ACCOUNT_SETTINGS)));
	fMenuBar->AddItem(fSettingsMenu);
	
	AddChild(fMenuBar);
}

void HTGTimeLineWindow::MessageReceived(BMessage *msg) {
	switch(msg->what) {
		case NEW_TWEET:
			newTweetWindow = new HTGNewTweetWindow(twitObj);
			newTweetWindow->Show();
			break;
		case REFRESH:
			friendsTimeLine->updateTimeLine();
			mentionsTimeLine->updateTimeLine();
			//publicTimeLine->updateTimeLine();
			break;
		case ACCOUNT_SETTINGS:
			accountSettingsWindow = new HTGAccountSettingsWindow();
			accountSettingsWindow->Show();
			break;
		case ABOUT:
			aboutWindow = new HTGAboutWindow();
			aboutWindow->Show();
			break;
		case B_CLOSE_REQUESTED:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}

HTGTimeLineWindow::~HTGTimeLineWindow() {
	
	be_app->PostMessage(B_QUIT_REQUESTED);
}