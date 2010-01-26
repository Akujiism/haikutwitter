/*
 * Copyright 2010 Martin Hebnes Pedersen, martinhpedersen @ "google mail"
 * All rights reserved. Distributed under the terms of the MIT License.
 */ 


#include "ListView.h"
#include "ScrollView.h"
#include "TabView.h"

#include "TimeLineParser.h"
#include "twitcurl/twitcurl.h"
#include "HTTweet.h"
#include "HTGTweetItem.h"

#ifndef HTG_TIMELINEVIEW_H
#define HTG_TIMELINEVIEW_H

const int32 TIMELINE_FRIENDS = 'TML';
const int32 TIMELINE_MENTIONS = 'MNTN';
const int32 TIMELINE_PUBLIC = 'PBLC';

status_t updateTimeLineThread(void *data);

class HTGTimeLineView : public BScrollView {
public:
	HTGTimeLineView(twitCurl *, const int32);
	void updateTimeLine();
	void AttachedToWindow();
	~HTGTimeLineView();
	
//private:
	bool waitingForUpdate;
	BListView *listView;
	BView *containerView;
	
	thread_id previousThread;
	
	TimeLineParser *timeLineParser;
	twitCurl *twitObj;
	int32 TYPE;
};
#endif