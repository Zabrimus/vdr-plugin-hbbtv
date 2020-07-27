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
         // inform browser about the channel switch

         // longName, Name => currentChannel
         // nid            => ??? (use 1 as default)
         // onid           => channel, Nid
         // sid            => channel, Sid
         // tsid           => channel, Tid
         // channelType    => HDTV 0x19, TV 0x01, Radio 0x02
         // idType         => ??? (use 15 as default)
         int channelType;

         if (strstr(currentChannel, "HD") != NULL) {
             channelType = 0x19;
         } else if (channel->Rid() > 0) {
             channelType = 0x02;
         } else {
             channelType = 0x01;
         }

         char *cmd;
         asprintf(&cmd, "CHANNEL {\"channelType\":%d,\"ccid\":\"ccid://1.0\",\"nid\":%d,\"dsd\":\"\",\"onid\":%d,\"tsid\":%d,\"sid\":%d,\"name\":\"%s\",\"longName\":\"%s\",\"description\":\"OIPF (SD&amp;S) - TCServiceData doesn’t support yet!\",\"authorised\":true,\"genre\":null,\"hidden\":false,\"idType\":%d,\"channelMaxBitRate\":0,\"manualBlock\":false,\"majorChannel\":1,\"ipBroadcastID\":\"rtp://1.2.3.4/\",\"locked\":false}", channelType, 1, channel->Nid(), channel->Tid(), channel->Sid(), currentChannel, currentChannel, 15);
         browserComm->SendToBrowser(cmd);
         free(cmd);

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
