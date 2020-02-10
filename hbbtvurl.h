/*
 * hbbtvurl.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#pragma once

#include "vdr/tools.h"

class cHbbtvURL : public cListObject {
  
private:
   uchar applicationId;
   uchar controlCode;
   uchar priority;
   cString name;
   cString urlBase;
   cString urlLoc;
   cString urlExt;
public:
   cHbbtvURL(uchar ApplicationId = 0, uchar ControlCode = 0, uchar Priority = 0, const char *Name = NULL, const char *UrlBase = NULL, const char *UrlLoc = NULL, const char *UrlExt = NULL);
   ~cHbbtvURL();
   virtual int Compare(const cListObject &ListObject) const;
   uchar ApplicationId(void) const { return applicationId; }
   uchar ControlCode(void) const { return controlCode; }
   uchar Priority(void) const { return priority; }
   const cString Name(void) const { return name; }
   const cString UrlBase(void) const { return urlBase; }
   const cString UrlLoc(void) const { return urlLoc; }
   const cString UrlExt(void) const { return urlExt; }
};


class cHbbtvURLs : public cList<cHbbtvURL> {
private:
 static cHbbtvURLs hbbtvURLs;
public:
   cHbbtvURLs();
   bool AddSortedUniqe(cHbbtvURL *HbbtvUrl);
        ///< add an URL to the sorted list if it does not already exist
        ///< it is considered unique if ControlCOde and ApplicationID is different 
   const char *Url(uchar ControlCode, uchar ApplicationId);
   static const cHbbtvURLs *HbbtvURLs();

};
