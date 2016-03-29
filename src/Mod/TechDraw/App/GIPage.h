/***************************************************************************
 *   Copyright (c) 2016                    Ian Rees <ian.rees@gmail.com>   *
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

#ifndef GIVIEWPAGE_HEADER
#define GIVIEWPAGE_HEADER

#include <QGraphicsView>

#include "DrawPage.h"
#include "GIBase.h"

namespace TechDraw {

class DrawTemplate;

class TechDrawExport GIPage : public QGraphicsView
{
    Q_OBJECT
public:
    GIPage(DrawPage *page, QWidget *parent = 0);
    virtual ~GIPage();

    /// Renders the page to SVG with filename.
    virtual void saveSvg(QString filename);

    /// Called after construction to attach all views that go on this page
    void attachViews();

    /// Getter for the views vector
    // TODO: Not convinced that the MDIView should be using this...
    const std::vector<GIBase *> & getViews() const { return views; }

    /// Setter for the views vector
    // TODO: Not convinced that the MDIView should be using this...
    void setViews(const std::vector<GIBase *> &view) {views = view; }

    /// Adds a new view to the page
    /*!
     * \return the size of the views vector.
     */
    int addView(GIBase *view);

    GIBase * findView(App::DocumentObject *obj) const;
    GIBase * findParent(GIBase *) const;

protected:
    /// Attaches view represented by obj to this
    virtual int attachView(App::DocumentObject *obj);
    
    /// As attachView (TODO: Perhaps roll this in to attachView?)
    virtual void attachTemplate(DrawTemplate *obj);

    /// The views that belong on this page
    std::vector<GIBase *> views;

    /// Pointer to DrawPage we are attached to
    // TODO: Replace this with a smart pointer
    DrawPage *m_page;    
};  // end class GIPage

};  // end namespace TechDraw

#endif // #ifndef GIVIEWPAGE_HEADER

