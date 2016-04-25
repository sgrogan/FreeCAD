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

#ifndef GIPAGE_HEADER
#define GIPAGE_HEADER

#include <QGraphicsScene>
#include <memory>

#include "../DrawPage.h"
#include "GIBase.h"
#include "GITemplate.h"

namespace TechDraw {

class DrawTemplate;

/// Contains the QGraphicsScene and GIBase-derived objects drawn on it
/*!
 * Note that this isn't intended as a "GUI" object in the sense that all user-
 * facing GUI interaction is handled through QGVPage which provides the QWidget
 * via QGraphicsView.  This class exists to allow rendering of a Drawing while
 * running without GUI via GIPageRenderer.
 */
class TechDrawExport GIPage
{
public:
    GIPage(DrawPage *page, QObject *sceneParent = 0);
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
    virtual int addView(GIBase *view);

    GIBase * findView(App::DocumentObject *obj) const;
    GIBase * findParent(GIBase *) const;

    /// Attaches view represented by obj to this
    virtual int attachView(App::DocumentObject *obj);

    /// Getter, for the current template
    GITemplate * getTemplate() const;

    /// Removes template from the page
    void removeTemplate();

    // Getter for DrawPage feature
    DrawPage* getDrawPage() {return m_page;};

protected:
    /// As attachView (TODO: Perhaps roll this in to attachView?)
    virtual void attachTemplate(DrawTemplate *obj);

    /// Helpful when changing templates
    void setSceneLimits();

    /// The views that belong on this page
    std::vector<GIBase *> views;

    /// Pointer to DrawPage we are attached to
    // TODO: Replace this with a smart pointer?
    DrawPage *m_page;

    /// Pointer to the currently active template
    std::unique_ptr<TechDraw::GITemplate> pageTemplate;

    /// Our raison d'etre - the graphics scene!
    QGraphicsScene *m_scene;
};  // end class GIPage

};  // end namespace TechDraw

#endif // #ifndef GIPAGE_HEADER
