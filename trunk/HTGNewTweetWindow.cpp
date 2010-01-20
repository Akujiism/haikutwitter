/*
 * Copyright 2010 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include "HTGNewTweetWindow.h"

HTGNewTweetWindow::HTGNewTweetWindow(twitCurl *twitObj) : BWindow(BRect(100, 100, 500, 200), "New tweet...", B_TITLED_WINDOW, B_NOT_RESIZABLE) {
	this->twitObj = twitObj;
	
	/*Add a grey view*/
	theView = new BView(Bounds(), "BackgroundView", B_NOT_RESIZABLE, B_WILL_DRAW);
	theView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	this->AddChild(theView);
	
	/*Set up text view*/
	message = new BTextView(BRect(5,5,395,65), "Text", BRect(5,5,395,65), B_NOT_RESIZABLE, B_WILL_DRAW);
	theView->AddChild(message);
	message->WindowActivated(true);
	
	/*Set up buttons*/
	postButton = new BButton(BRect(3, 70, 90, 90), NULL, "Post", new BMessage(POST));
	cancelButton = new BButton(BRect(93, 70, 180, 90), NULL, "Cancel", new BMessage(CANCEL));
	theView->AddChild(postButton);
	theView->AddChild(cancelButton);
}

void HTGNewTweetWindow::postTweet() {
	std::string replyMsg( "" );
	std::string postMsg(message->Text());
	if( twitObj->statusUpdate(postMsg) )  {
		printf( "Status update: OK\n" );
	}
	else {
		twitObj->getLastCurlError( replyMsg );
		printf( "\ntwitterClient:: twitCurl::updateStatus error:\n%s\n", replyMsg.c_str() );
		BAlert *theAlert = new BAlert("Oops, sorry!", replyMsg.c_str(), "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT); 	
	}
}

void HTGNewTweetWindow::MessageReceived(BMessage *msg) {
	switch(msg->what) {
		case POST:
			this->postTweet();
			this->Close();
			break;
		case CANCEL:
			this->Close();
		default:
			BWindow::MessageReceived(msg);
	}
}

HTGNewTweetWindow::~HTGNewTweetWindow() {
	message->RemoveSelf();
	delete message;
	
	postButton->RemoveSelf();
	delete postButton;
	
	cancelButton->RemoveSelf();
	delete cancelButton;
	
	theView->RemoveSelf();
	delete theView;
}
