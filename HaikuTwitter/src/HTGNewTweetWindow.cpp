/*
 * Copyright 2010-2012 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * Multibyte string count by Yoshida Mitsunori
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include "HTGNewTweetWindow.h"

using namespace std;

static size_t WriteMemoryCallback(void*, size_t, size_t, void*);

HTGNewTweetWindow::HTGNewTweetWindow(twitCurl *twitObj, BHandler *parent)
	: BWindow(BRect(100, 100, 500, 200), "What's Happening?", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	this->twitObj = twitObj;
	this->tweetId = string("");
	
	/*Add a grey view*/
	theView = new BView(Bounds(), "BackgroundView", B_NOT_RESIZABLE, B_WILL_DRAW);
	theView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	this->AddChild(theView);
	
	/*Set up text view*/
	message = new HTGTextView(BRect(5,5,395,65), "Text", BRect(5,5,380,65), B_NOT_RESIZABLE, B_WILL_DRAW);
	theView->AddChild(message);
	message->WindowActivated(true);
	
	/*Set up buttons*/
	postButton = new BButton(BRect(3, 70, 90, 90), NULL, "Tweet", new BMessage(POST));
	cancelButton = new BButton(BRect(93, 70, 180, 90), NULL, "Cancel", new BMessage(CANCEL));
	shortenButton = new BButton(BRect(203, 70, 313, 90), NULL, "Shorten URLs", new BMessage(SHORTEN));
	postButton->SetEnabled(false);
	theView->AddChild(postButton);
	theView->AddChild(shortenButton);
	theView->AddChild(cancelButton);
	
	/*Set up symbol counter*/
	counterView = new BStringView(BRect(350, 75, 400, 95), "Counter", "140");
	counterView->SetHighColor(128, 128, 128);
	theView->AddChild(counterView);
	
	this->parent = parent;
	
	message->MakeFocus();
}

void
HTGNewTweetWindow::SetText(const char *text)
{
	message->SetText(text);
	this->MessageReceived(new BMessage('UPDT')); //Update counter
}

void
HTGNewTweetWindow::setTweetId(const char* tweetId)
{
	if(tweetId != NULL)
		this->tweetId = string(tweetId);
}

void
HTGNewTweetWindow::postTweet()
{
	std::string replyMsg( "" );
	std::string postMsg = urlEncode(message->Text());
	if( twitObj->statusUpdate(postMsg, tweetId) )  {
		printf( "Status update: OK\n" );
		twitObj->getLastWebResponse(replyMsg);
		int errorStart = replyMsg.find("<error>");
		int errorEnd = replyMsg.find("<\error>");
		if(errorStart-errorEnd > 0)
			HTGErrorHandling::displayError(replyMsg.substr(errorStart, errorEnd-errorStart).c_str());
		else { // Send "status updated" to parent, so we can refresh timelines
			BMessage* statusUpdated = new BMessage(STATUS_UPDATED);
			statusUpdated->AddString("content", message->Text());
			parent->MessageReceived(statusUpdated);
		}
	}
	else {
		//twitObj->getLastCurlError( replyMsg );
		HTGErrorHandling::displayError("Oops, sorry! Looks like curl failed at some point..."); 	
	}
}

std::string
HTGNewTweetWindow::urlEncode(const char* input)
{
	std::string output(input);
	
	for(size_t i = 0; i < output.length(); i++) {
		if(output[i] == '%')
			output.replace(i, 1, "%25");
		if(output[i] == '&')
			output.replace(i, 1, "%26");
		if(output[i] == '+')
			output.replace(i, 1, "%2B");
		if(output[i] == '@')
			output.replace(i, 1, "%40");
		if(output[i] == '?')
			output.replace(i, 1, "%3F");
		if(output[i] == '=')
			output.replace(i, 1, "%3D");
		if(output[i] == '$')
			output.replace(i, 1, "%24");
		if(output[i] == '/')
			output.replace(i, 1, "%2F");
		if(output[i] == ':')
			output.replace(i, 1, "%3A");
		if(output[i] == ';')
			output.replace(i, 1, "%3B");
		if(output[i] == ',')
			output.replace(i, 1, "%2C");
	}
	
	return output;
}

void
HTGNewTweetWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case POST:
			this->postTweet();
			this->Close();
			break;
		case CANCEL:
			this->Close();
			break;
		case UPDATED:
			this->updateCounter();
			break;
		case SHORTEN:
			this->shortenAllUrls();
			this->updateCounter();
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}

void
HTGNewTweetWindow::shortenAllUrls()
{
	size_t pos = 0;
	std::string theText(message->Text());
	
	while(pos != std::string::npos) {
		pos = theText.find("://", pos);
		if(pos != std::string::npos) {
			size_t start = pos;
			size_t end = pos;
			while(start != 0 && theText[start] != ' ') {
				start--;
			}
			while(end < theText.length() && theText[end] != ' ') {
				end++;
			}
			const char *shortUrl = shortenUrl(theText.substr(start+1, end-start-1).c_str());
			message->SetText(theText.replace(start+1, end-start-1, shortUrl).c_str());
			pos = end;
		}
	}	
}

void
HTGNewTweetWindow::updateCounter()
{
	char counterString[32];
	int symbolsLeft =  NUMBER_OF_ALLOWED_CHARS;
  	
	/*Calculate the number of characters left*/
	int utf8length = 0;
	uchar c;
	for(int i=0; (c=message->ByteAt(i))!='\0'; i++) {
		if ((c&0x80) == 0) {
			// UTF-8 single byte char
			utf8length++;
		} else if ((c&0xc0) != 0x80) {
			// first byte of UTF-8 multi byte character
			utf8length++;
		}
	}
	symbolsLeft -= utf8length;
	sprintf(counterString, "%i", symbolsLeft);
	
	/*Check symbolsLeft, disable/enable post button and change counter color.*/
	if(symbolsLeft < 0) {
		counterView->SetHighColor(255, 0, 0);
		postButton->SetEnabled(false);
	}
	else if(symbolsLeft < 140) {
		counterView->SetHighColor(128, 128, 128);
		if(symbolsLeft < 140)
			postButton->SetEnabled(true);
	}
	else if(symbolsLeft >= 140)
		postButton->SetEnabled(false);
	
	counterView->SetText(counterString);
}

const char*
HTGNewTweetWindow::shortenUrl(const char *longUrl)
{
	// Use bit.ly for shortening
	std::string url("http://api.bit.ly/shorten?login=");
	const char *user = "haikutwitter";
	const char *apik = "R_d1baa8ab95fe79f4c7b58cb4db23d1dd";
	const char *version = "2.0.1";
	url.append(user);
	url.append("&apiKey=");
	url.append(apik);
	url.append("&version=");
	url.append(version);
	url.append("&longUrl=");
	url.append(longUrl);
		
	CURL *curl_handle;
	BMallocIO *mallocIO = new BMallocIO();
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	
	/*send all data to this function*/
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	
	/*we pass out 'mallocIO' object to the callback function*/
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)mallocIO);
	
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "haikutwitter-agent/1.0");
	
	/*get the data*/
	if(curl_easy_perform(curl_handle) < 0)
		std::cout << "libcURL: Unable to retrieve result from bit.ly" << std::endl;
	
	/*cleanup curl stuff*/
	curl_easy_cleanup(curl_handle);
	
	/*Copy downloaded data into webString*/
	std::string webString((char *)mallocIO->Buffer());
	
	/*Search for "shortUrl"*/
	size_t startPos = webString.find("\"shortUrl");
	if(startPos == std::string::npos) {
		std::cout << "bi.ly/shorten failed." << std::endl;
		delete mallocIO;
		return longUrl;
		
	}
	
	/*Copy the new url into char**/
	char *returnString = new char[21];
	for(int i = 0; i < 20; i++) {
		returnString[i] = webString[startPos+13+i];
	} 
	returnString[20] = '\0';
	
	/*Delete the buffer*/
	delete mallocIO;
	
	return returnString;
}

/*Callback function for cURL*/
static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size *nmemb;
	BMallocIO *mallocIO = (BMallocIO *)data;
	
	return mallocIO->Write(ptr, realsize);
}

HTGNewTweetWindow::~HTGNewTweetWindow()
{
	message->RemoveSelf();
	delete message;
	
	postButton->RemoveSelf();
	delete postButton;
	
	cancelButton->RemoveSelf();
	delete cancelButton;
	
	shortenButton->RemoveSelf();
	delete shortenButton;
	
	theView->RemoveSelf();
	delete theView;
}
