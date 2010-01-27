/*
 * Copyright 2010 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */

 
#include "HTGTweetTextView.h"

HTGTweetTextView::HTGTweetTextView(BRect frame, const char *name, BRect textRect, uint32 resizingMode, uint32 flags) : BTextView(frame, name, textRect, resizingMode, flags) {
		
}
	
HTGTweetTextView::HTGTweetTextView(BRect frame, const char *name, BRect textRect, const BFont* font, const rgb_color* color, uint32 resizingMode, uint32 flags) : BTextView(frame, name, textRect, font, color, resizingMode, flags) {
		
}

void HTGTweetTextView::MouseDown(BPoint point) {	
	BMenuItem* selected;
	
	ConvertToScreen(&point);
	
	BPopUpMenu *myPopUp = new BPopUpMenu("TweetOptions", false, true, B_ITEMS_IN_COLUMN);
	
	myPopUp->AddItem(new BMenuItem("Retweet...", new BMessage(GO_RETWEET)));
	myPopUp->AddItem(new BMenuItem("Reply...", new BMessage(GO_REPLY)));
	myPopUp->AddSeparatorItem();
	
	BList *screenNameList = this->getScreenNames();
	for(int i = 0; i < screenNameList->CountItems(); i++)
		myPopUp->AddItem((BMenuItem *)screenNameList->ItemAt(i));
	myPopUp->AddSeparatorItem();
	
	BList *urlList = this->getUrls();
	for(int i = 0; i < urlList->CountItems(); i++)
		myPopUp->AddItem((BMenuItem *)urlList->ItemAt(i));
	
	selected = myPopUp->Go(point);
	
	if (selected) {
      this->MessageReceived(selected->Message());
   	}
	
	delete myPopUp;
	delete screenNameList;
	delete urlList;
}

BList* HTGTweetTextView::getScreenNames() {
	BList *theList = new BList();
	
	std::string tweetersName(this->Name());
	tweetersName.insert(0, "@");
	tweetersName.append("...");
	BMessage *firstMessage = new BMessage(GO_TO_USER);
	firstMessage->AddString("ScreenName", this->Name());
	theList->AddItem(new BMenuItem(tweetersName.c_str(), firstMessage));
			
	for(int i = 0; Text()[i] != '\0'; i++) {
		if(Text()[i] == '@') {
			i++; //Skip leading '@'
			std::string newName("");
			while(isValidScreenNameChar(Text()[i])) {
				newName.append(1, Text()[i]);
				i++;
			}
			const char *screenName = newName.c_str();
			newName.insert(0, "@");
			newName.append("...");
			BMessage *theMessage = new BMessage(GO_TO_USER);
			theMessage->AddString("ScreenName", screenName);
			theList->AddItem(new BMenuItem(newName.c_str(), theMessage));
		}
	}
	
	return theList;
}

BList* HTGTweetTextView::getUrls() {
	BList *theList = new BList();
	size_t pos = 0;
	std::string theText(this->Text());
	
	while(pos != std::string::npos) {
		pos = theText.find("://", pos);
		if(pos != std::string::npos) {
			int start = pos;
			int end = pos;
			while(start >= 0 && theText[start] != ' ') {
				start--;
			}
			while(end < theText.length() && theText[end] != ' ') {
				end++;
			}
			if(end == theText.length()-2) //For some reason, we have to do this.
				end--;
			BMessage *theMessage = new BMessage(GO_TO_URL);
			theMessage->AddString("url", theText.substr(start+1, end-start-1).c_str());
			theList->AddItem(new BMenuItem(theText.substr(start+1, end-start-1).c_str(), theMessage));
			pos = end;
		}
	}
	
	return theList;
}

bool HTGTweetTextView::isValidScreenNameChar(const char& c) {
	if(c <= 'z' && c >= 'a')
		return true;
	if(c <= 'Z' && c >= 'A')
		return true;
	if(c <= '9' && c >= '0')
		return true;
	if(c == '_')
		return true;
	
	return false;
}

void HTGTweetTextView::sendRetweetMsgToParent() {
	BMessage *retweetMsg = new BMessage(NEW_TWEET);
	std::string RTString(this->Text());
	RTString.insert(0, " ");
	RTString.insert(0, this->Name());
	RTString.insert(0, "@");
	RTString.insert(0, "RT ");
	retweetMsg->AddString("text", RTString.c_str());
	BTextView::MessageReceived(retweetMsg);
}

void HTGTweetTextView::sendReplyMsgToParent() {
	BMessage *replyMsg = new BMessage(NEW_TWEET);
	std::string theString(" ");
	theString.insert(0, this->Name());
	theString.insert(0, "@");
	replyMsg->AddString("text", theString.c_str());
	BTextView::MessageReceived(replyMsg);
}

void HTGTweetTextView::MessageReceived(BMessage *msg) {
	const char* url_label = "url";
	const char* name_label = "screenName";
	std::string newTweetAppend(" ");
	switch(msg->what) {
		case GO_TO_USER:
			std::cout << "Go to user: This function is not implemented yet..." << std::endl;
			break;
		case GO_RETWEET:
			this->sendRetweetMsgToParent();
			break;
		case GO_REPLY:
			this->sendReplyMsgToParent();
			break;
		case GO_TO_URL:
			this->openUrl(msg->FindString(url_label, (int32)0));
			break;
		default:
			BTextView::MessageReceived(msg);
	}
}

void HTGTweetTextView::openUrl(const char *url) {
	// be lazy and let /bin/open open the URL
	entry_ref ref;
	if (get_ref_for_path("/bin/open", &ref))
		return;
		
	const char* args[] = { "/bin/open", url, NULL };
	be_roster->Launch(&ref, 2, args);
}
