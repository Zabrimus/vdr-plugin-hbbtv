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


cHbbtvDeviceStatus::cHbbtvDeviceStatus()
{
   device = NULL;
   aitFilter = NULL;
   sid = -1;
}

cHbbtvDeviceStatus::~cHbbtvDeviceStatus()
{
}

void cHbbtvDeviceStatus::ChannelSwitch(const cDevice * vdrDevice, int channelNumber, bool LiveView)
{  // Indicates a channel switch on the given DVB device.
   // If ChannelNumber is 0, this is before the channel is being switched,
   // otherwise ChannelNumber is the number of the channel that has been switched to.
   // LiveView tells whether this channel switch is for live viewing.
   
   if (LiveView) {
      if (device) {
         isyslog("[hbbtv] Detaching HbbTV ait filter from device %d", device->CardIndex()+1);
         device->Detach(aitFilter);
         device = NULL;
         aitFilter = NULL;
         sid = -1;

         //clear URL List
         cHbbtvURLs *hbbtvURLs = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();
         hbbtvURLs->Clear();
      }
      if (channelNumber) {
         device = cDevice::ActualDevice();
#if APIVERSNUM >= 20301
         LOCK_CHANNELS_READ
         sid = Channels->GetByNumber(channelNumber)->Sid();
#else
         sid = Channels.GetByNumber(channelNumber)->Sid();
#endif
         device->AttachFilter(aitFilter = new cAitFilter(sid));
         isyslog("[hbbtv] Attached HbbTV ait filter to device %d, vdrDev=%d actDev=%d, Sid=0x%04x", device->CardIndex()+1, vdrDevice->CardIndex()+1,cDevice::ActualDevice()->CardIndex()+1, sid);
      }
   }
}
