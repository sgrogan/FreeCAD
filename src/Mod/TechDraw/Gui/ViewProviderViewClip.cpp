/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
//#include <Gui/SoFCSelection.h>
#include <Gui/Selection.h>

#include <Mod/Drawing/App/DrawViewClip.h>
#include "ViewProviderView.h"
#include "ViewProviderViewClip.h"


using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderDrawingClip, TechDrawGui::ViewProviderView)

ViewProviderDrawingClip::ViewProviderDrawingClip()
{
    sPixmap = "actions/techdraw-clip";

    // Do not show in property editor
    DisplayMode.StatusBits.set(3, true);
}

ViewProviderDrawingClip::~ViewProviderDrawingClip()
{
}

void ViewProviderDrawingClip::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderDrawingClip::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDrawingClip::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList;
    return StrList;
}

std::vector<App::DocumentObject*> ViewProviderDrawingClip::claimChildren(void) const
{
    // Collect any child views
    // for Clip, valid children are any View in Views
    const std::vector<App::DocumentObject *> &views = getObject()->Views.getValues();
    return views;
}

void ViewProviderDrawingClip::show(void)
{
    ViewProviderView::show();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<TechDraw::DrawViewClip*>(obj)->Visible.setValue(true);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
}

void ViewProviderDrawingClip::hide(void)
{
    ViewProviderView::hide();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<TechDraw::DrawViewClip*>(obj)->Visible.setValue(false);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
}

bool ViewProviderDrawingClip::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDrawingClip::startRestoring()
{
    // do nothing
}

void ViewProviderDrawingClip::finishRestoring()
{
    // do nothing
}

void ViewProviderDrawingClip::updateData(const App::Property*)
{
}

TechDraw::DrawViewClip* ViewProviderDrawingClip::getObject() const
{
    return dynamic_cast<TechDraw::DrawViewClip*>(pcObject);
}
