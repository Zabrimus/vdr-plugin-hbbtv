/**
 *  VDR HbbTV Plugin
 *
 *  cefhbbtvpage.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef CEFHBBTVPAGE_H
#define CEFHBBTVPAGE_H

#include <map>
#include <vdr/osdbase.h>
#include "browser.h"

class CefHbbtvPage : public cOsdObject {

private:
    cOsd *osd;
    std::string hbbtvUrl;

    std::map<eKeys, std::string> keyMapping;

    void initKeyMapping();
    void SendKey(std::string key);

public:
    CefHbbtvPage();
    ~CefHbbtvPage() override;
    void LoadUrl(std::string _hbbtvUrl);
    void Show() override;
    void Hide();

    eOSState ProcessKey(eKeys Key) override;
};


#endif // CEFHBBTVPAGE_H
