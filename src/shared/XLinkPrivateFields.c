// Copyright (C) 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <stdio.h>
#include <string.h>
#include "stdlib.h"

#include "XLinkPrivateFields.h"
#include "XLinkPrivateDefines.h"
#include "XLinkErrorUtils.h"

#ifdef MVLOG_UNIT_NAME
#undef MVLOG_UNIT_NAME
#define MVLOG_UNIT_NAME xLink
#endif

#include "XLinkLog.h"

xLinkDesc_t* getLinkById(linkId_t id)
{
    XLINK_RET_ERR_IF(pthread_mutex_lock(&availableXLinksMutex) != 0, NULL);

    int i;
    for (i = 0; i < MAX_LINKS; i++) {
        if (availableXLinks[i].id == id) {
            XLINK_RET_ERR_IF(pthread_mutex_unlock(&availableXLinksMutex) != 0, NULL);
            return &availableXLinks[i];
        }
    }

    XLINK_RET_ERR_IF(pthread_mutex_unlock(&availableXLinksMutex) != 0, NULL);
    return NULL;
}

xLinkDesc_t* getLink(void* fd)
{

    XLINK_RET_ERR_IF(pthread_mutex_lock(&availableXLinksMutex) != 0, NULL);

    int i;
    for (i = 0; i < MAX_LINKS; i++) {
        if (availableXLinks[i].deviceHandle.xLinkFD == fd) {
            XLINK_RET_ERR_IF(pthread_mutex_unlock(&availableXLinksMutex) != 0, NULL);
            return &availableXLinks[i];
        }
    }

    XLINK_RET_ERR_IF(pthread_mutex_unlock(&availableXLinksMutex) != 0, NULL);
    return NULL;
}

streamId_t getStreamIdByName(xLinkDesc_t* link, const char* name)
{
    streamDesc_t* stream = getStreamByName(link, name);

    if (stream) {
        streamId_t id = stream->id;
        releaseStream(stream);
        return id;
    }

    return INVALID_STREAM_ID;
}

streamDesc_t* getStreamById(void* fd, streamId_t id)
{
    XLINK_RET_ERR_IF(id == INVALID_STREAM_ID, NULL);
    xLinkDesc_t* link = getLink(fd);
    XLINK_RET_ERR_IF(link == NULL, NULL);
    int stream;
    for (stream = 0; stream < XLINK_MAX_STREAMS; stream++) {
        if (link->availableStreams[stream].id == id) {

            if(XLink_sem_wait(&link->availableStreams[stream].sem)){
                return NULL;
            }

            return &link->availableStreams[stream];
        }
    }
    return NULL;
}

streamDesc_t* getStreamByName(xLinkDesc_t* link, const char* name)
{
    XLINK_RET_ERR_IF(link == NULL, NULL);
    int stream;
    for (stream = 0; stream < XLINK_MAX_STREAMS; stream++) {
        if (link->availableStreams[stream].id != INVALID_STREAM_ID &&
            strcmp(link->availableStreams[stream].name, name) == 0) {

            if(XLink_sem_wait(&link->availableStreams[stream].sem)){
                return NULL;
            }

            return &link->availableStreams[stream];
        }
    }
    return NULL;
}

void releaseStream(streamDesc_t* stream)
{
    if (stream && stream->id != INVALID_STREAM_ID) {
        XLink_sem_post(&stream->sem);
    }
    else {
        mvLog(MVLOG_DEBUG,"trying to release a semaphore for a released stream\n");
    }
}

xLinkState_t getXLinkState(xLinkDesc_t* link)
{
    XLINK_RET_ERR_IF(link == NULL, XLINK_NOT_INIT);
    mvLog(MVLOG_DEBUG,"%s() link %p link->peerState %d\n", __func__,link, link->peerState);
    return link->peerState;
}
