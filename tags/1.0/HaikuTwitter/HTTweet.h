/*
 * Copyright 2010-2011 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 

#ifndef HT_TWEET_H
#define HT_TWEET_H

#include <iostream>
#include <string.h>
#include <string>
#include <View.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <Bitmap.h>
#include <Archivable.h>
#include <File.h>
#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <Application.h>
#include <Resources.h>
#include <Roster.h>

#include <ctime>
#include <sstream>
#include <curl/curl.h>

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
status_t _threadDownloadBitmap(void *);

using namespace std;

struct DateStruct {
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int year;
};

class HTTweet : public BArchivable {
public:
	HTTweet();
	HTTweet(HTTweet *originalTweet);
	HTTweet(BMessage* archive);
	~HTTweet();
	status_t Archive(BMessage* archive, bool deep);
	BArchivable* Instantiate(BMessage* archive);
	bool operator<(const HTTweet &b) const;
	BView* getView();
	const string getScreenName() const;
	const string getRetweetedBy() const;
	const string getFullName();
	const string getText();
	const string getProfileImageUrl();
	const string getRelativeDate() const;
	const string getSourceName();
	time_t getUnixTime() const;
	BBitmap* getBitmap();
	BBitmap getBitmapCopy();
	struct DateStruct getDate() const;
	static int sortByDateFunc(const void*, const void*);
	static BBitmap* defaultBitmap();
	void setView(BView *);
	void downloadBitmap(bool async = false);
	void setRetweetedBy(string);
	void setScreenName(string);
	void setFullName(string);
	void setText(string);
	void setProfileImageUrl(string);
	void setDate(time_t);
	void setId(uint64);
	void setSourceName(string);
	void setBitmap(BBitmap *);
	void waitUntilDownloadComplete();
	bool isDownloadingBitmap();
	uint64 getId();
	const char* getIdString();
	bool following();
	void setFollowing(bool);
	
	/*This must be public (threads)*/
	bool bitmapDownloadInProgress;//Archived
	BMallocIO* bitmapData;
	
private:
	const char* monthToString(int month) const;
	BView *view;				//Not archived
	thread_id downloadThread;	//Not archived
	BBitmap *imageBitmap;		//Archived
	string screenName;			//Archived
	string fullName;			//Archived
	string text;				//Archived
	string profileImageUrl;		//Archived
	string sourceName;			//Archived
	string retweetedBy;
	struct DateStruct date;		//Not archived
	uint64 id;					//Archived
	bool isFollowing;			//Archived
};
#endif