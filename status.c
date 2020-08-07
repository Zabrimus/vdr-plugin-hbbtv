/*
 * status.c: keep track of VDR device status
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "status.h"
#include "ait.h"
#include "hbbtvurl.h"
#include <vdr/channels.h>
#include "browsercommunication.h"
#include "globals.h"

cHbbtvDeviceStatus::cHbbtvDeviceStatus()
{
   device = NULL;
   aitFilter = NULL;
   sid = -1;
}

cHbbtvDeviceStatus::~cHbbtvDeviceStatus() {
    device->Detach(aitFilter);
}

void cHbbtvDeviceStatus::ChannelSwitch(const cDevice * vdrDevice, int channelNumber, bool LiveView)
{  // Indicates a channel switch on the given DVB device.
   // If ChannelNumber is 0, this is before the channel is being switched,
   // otherwise ChannelNumber is the number of the channel that has been switched to.
   // LiveView tells whether this channel switch is for live viewing.
   cHbbtvURLs *urls = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();

   if (LiveView) {
      if (device) {
         isyslog("[hbbtv] Detaching HbbTV ait filter from device %d", device->CardIndex()+1);
         device->Detach(aitFilter);
         device = NULL;
         aitFilter = NULL;
         sid = -1;

         //clear URL List
         urls->Clear();
      }

      if (channelNumber) {
         device = cDevice::ActualDevice();

#if APIVERSNUM >= 20301
         LOCK_CHANNELS_READ
         auto channel = Channels->GetByNumber(channelNumber);
         sid = channel->Sid();
         const char* currentChannel = channel->Name();
#else
         auto channel = Channels->GetByNumber(channelNumber);
         sid = channel->Sid();
         const char* currentChannel = channel->Name();
#endif

         sendChannelToBrowser(channelNumber);

         // find all well known URLs
         char* search;
         asprintf(&search, "%s~~~", currentChannel);

         cStringList *allUrls = cHbbtvURLs::AllURLs();
         for (int i = 0; i < allUrls->Size(); i++) {
            if (strncmp((*allUrls)[i], search, strlen(search)) == 0) {
                cHbbtvURL *url = cHbbtvURL::FromString((*allUrls)[i] + strlen(search));
                urls->AddSortedUniqe(url);
            }
         }

         free(search);

         device->AttachFilter(aitFilter = new cAitFilter(sid));
         isyslog("[hbbtv] Attached HbbTV ait filter to device %d, vdrDev=%d actDev=%d, Sid=0x%04x", device->CardIndex()+1, vdrDevice->CardIndex()+1,cDevice::ActualDevice()->CardIndex()+1, sid);
      }
   }
}
