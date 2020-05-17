/*
 * hbbtvurl.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <string>
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
   } else {
      return applicationId - hbbtvUrl->ApplicationId();
   }
}

char* cHbbtvURL::ToString() {
    char *out;
    asprintf(&out, "%d~~~%d~~~%d~~~%s~~~%s~~~%s~~~%s", applicationId, controlCode, priority, *name, *urlBase, *urlLoc, *urlExt);
    return out;
}

cHbbtvURL* cHbbtvURL::FromString(char* input) {
    cHbbtvURL* url = new cHbbtvURL();
    size_t pos = 0;
    std::string token;

    std::string s(input);
    int idx = 0;
    while ((pos = s.find("~~~")) != std::string::npos) {
        token = s.substr(0, pos);

        if (token.length() > 0) {
            switch (idx) {
                case 0:
                    url->applicationId = std::stoi(token);
                    break;

                case 1:
                    url->controlCode = std::stoi(token);
                    break;

                case 2:
                    url->priority = std::stoi(token);
                    break;

                case 3:
                    url->name = strdup(token.c_str());
                    break;

                case 4:
                    url->urlBase = strdup(token.c_str());
                    break;

                case 5:
                    url->urlLoc = strdup(token.c_str());
                    break;

                case 6:
                    url->urlExt = strdup(token.c_str());
                    break;

                default:
                    break;
            }
        }
        s.erase(0, pos + 3);
        idx++;
    }

    return url;
}

// --- cHbbtvURLs ---------------------------------------
cHbbtvURLs cHbbtvURLs::hbbtvURLs;
cStringList cHbbtvURLs::allURLs;

cHbbtvURLs::cHbbtvURLs()
{
  return;
}

const cHbbtvURLs *cHbbtvURLs::HbbtvURLs()
{
   return &hbbtvURLs;
}

cStringList *cHbbtvURLs::AllURLs()
{
    return &allURLs;
}

bool cHbbtvURLs::AddSortedUniqe(cHbbtvURL *newUrl)
{
   cHbbtvURL *lastUrl, *url;
   url = lastUrl = hbbtvURLs.First();

   if (!url) {
      hbbtvURLs.Add(newUrl);
      return true;
   }

   url = hbbtvURLs.First();
   while (url) {
       if (url->Compare(*newUrl) == 0) {
           return false;
       } else {
           url = hbbtvURLs.Next(url);
       }
   }

    url = hbbtvURLs.First();
    while (url && url->Compare(*newUrl) < 0) {
      lastUrl = url;
      url = hbbtvURLs.Next(url);
   }

   if (!url) {
       hbbtvURLs.Add(newUrl);
       return true;
   } else if (lastUrl->Compare(*newUrl) != 0) {
       if (lastUrl && lastUrl->Compare(*newUrl) > 0) {
           hbbtvURLs.Ins(newUrl, lastUrl);
       } else {
           hbbtvURLs.Add(newUrl, lastUrl);
       }

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
