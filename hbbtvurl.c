/*
 * hbbtvurl.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "hbbtvurl.h"
#include "libsi/si.h"
#include "vdr/tools.h"
#define DSYSLOG(x...)    dsyslog(x);

// --- cHbbtvURL ---------------------------------------
cHbbtvURL::cHbbtvURL(uchar ApplicationId, uchar ControlCode, uchar Priority, const char *Name, const char *UrlBase, const char *UrlLoc, const char *UrlExt)
{
   applicationId = ApplicationId;
   controlCode = ControlCode;
   priority = Priority;
   name    = Name;
   urlBase = UrlBase;
   urlLoc  = UrlLoc;
   urlExt  = UrlExt;
}

cHbbtvURL::~cHbbtvURL()
{
}

int cHbbtvURL::Compare(const cListObject &ListObject) const
{
   cHbbtvURL *hbbtvUrl = (cHbbtvURL *)&ListObject;
   if (controlCode != hbbtvUrl->ControlCode()) {
      return controlCode - hbbtvUrl->ControlCode();
   }
   else
      return applicationId - hbbtvUrl->ApplicationId();
}


// --- cHbbtvURLs ---------------------------------------
cHbbtvURLs cHbbtvURLs::hbbtvURLs;

cHbbtvURLs::cHbbtvURLs()
{
  return;
}

const cHbbtvURLs *cHbbtvURLs::HbbtvURLs()
{
   return &hbbtvURLs;
}

bool cHbbtvURLs::AddSortedUniqe(cHbbtvURL *newUrl)
{
   cHbbtvURL *lastUrl, *url;
   url = lastUrl = hbbtvURLs.First();

   if (!url)
   {
      hbbtvURLs.Add(newUrl);
      return true;
   }
   
   while (url && 0 > url->Compare(*newUrl))
   {
      lastUrl = url;
      url = hbbtvURLs.Next(url);
   }
   
   if (!url || 0 != url->Compare(*newUrl)) {
      hbbtvURLs.Add(newUrl, lastUrl);
      return true;
   }

   return false;
}

const char *cHbbtvURLs::Url(uchar ControlCode, uchar ApplicationId)
{
   cHbbtvURL *url = hbbtvURLs.First();
   while (url && ControlCode != url->ControlCode() && ApplicationId != url->ApplicationId())
      url = hbbtvURLs.Next(url);
   
   if (!url)
      return NULL;
   else
      return (*cString::sprintf("%s%s",*url->UrlBase(),  *url->UrlLoc()));
   }
