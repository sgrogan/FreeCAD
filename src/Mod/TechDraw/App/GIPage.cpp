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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QSvgGenerator>
#endif // #ifndef _PreComp_

#include "DrawSVGTemplate.h"
#include "DrawViewCollection.h"
#include "DrawViewDimension.h"

#include "GIPage.h"
#include <QDebug>   // TODO: Remove this
using namespace TechDraw;

GIPage::GIPage(DrawPage *page, QWidget *parent)
    : QGraphicsView(parent),
      m_page(page)
{
    assert(m_page);
    m_page->registerGi(this);    
    
    setScene(new QGraphicsScene(this));

}

// Note that if the GUI is running, the calls to attachView() will
// go to the overrides in QGVPage.
void GIPage::attachViews()
{
    // A fresh page is added and we iterate through its collected children and add these to Canvas View
    // if docobj is a featureviewcollection (ex orthogroup), add its child views. if there are ever children that have children,
    // we'll have to make this recursive. -WF
    const std::vector<App::DocumentObject*> &grp = m_page->Views.getValues();
    std::vector<App::DocumentObject*> childViews;
    for (std::vector<App::DocumentObject*>::const_iterator it = grp.begin();it != grp.end(); ++it) {
        attachView(*it);
        DrawViewCollection* collect = dynamic_cast<DrawViewCollection *>(*it);
        if (collect) {
            childViews = collect->Views.getValues();
            for (std::vector<App::DocumentObject*>::iterator itChild = childViews.begin(); itChild != childViews.end(); ++itChild) {
                attachView(*itChild);
            }
        }
    }
/*
    //when restoring, it is possible for a Dimension to be loaded before the ViewPart it applies to
    //therefore we need to make sure parentage of the graphics representation is set properly. bit of a kludge.
    const std::vector<QGIView *> &allItems = m_view->getViews();
    int dimItemType = QGraphicsItem::UserType + 106;

    std::vector<QGIView *>::const_iterator itInspect;
    for (itInspect = allItems.begin(); itInspect != allItems.end(); itInspect++) {
        if (((*itInspect)->type() == dimItemType) && (!(*itInspect)->group())) {
            QGIView* parent = m_view->findParent((*itInspect));
            if (parent) {
                QGIViewDimension* dim = dynamic_cast<QGIViewDimension*>((*itInspect));
                m_view->addDimToParent(dim,parent);
            }
        }
    }
*/
    App::DocumentObject *obj = m_page->Template.getValue();
    if(obj && obj->isDerivedFrom(DrawTemplate::getClassTypeId())) {
        DrawTemplate *pageTemplate = dynamic_cast<DrawTemplate *>(obj);
        attachTemplate(pageTemplate);
    }
}

int GIPage::addView(GIBase *view)
{
    views.push_back(view);

    // Find if it belongs to a parent
    GIBase *parent = 0;
    parent = findParent(view);

    QPointF viewPos(view->getViewObject()->X.getValue(),
                    view->getViewObject()->Y.getValue() * -1);

    if(parent) {
        // Transfer the child vierw to the parent
        QPointF posRef(0.,0.);

        QPointF mapPos = view->mapToItem(parent, posRef);              //setPos is called later.  this doesn't do anything?
        view->moveBy(-mapPos.x(), -mapPos.y());

        parent->addToGroup(view);
    }

    view->setPos(viewPos);

    return views.size();
}

GIBase * GIPage::findView(App::DocumentObject *obj) const
{
  if(scene()) {
    const std::vector<GIBase *> qviews = views;
    for(std::vector<GIBase *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
          TechDraw::DrawView *fview = (*it)->getViewObject();
          if(fview && strcmp(obj->getNameInDocument(), fview->getNameInDocument()) == 0)
              return *it;
      }
  }
    return 0;
}

GIBase * GIPage::findParent(GIBase *view) const
{
    const std::vector<GIBase *> qviews = views;
    TechDraw::DrawView *myView = view->getViewObject();

    //If type is dimension we check references first
    TechDraw::DrawViewDimension *dim = 0;
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(myView);

    if(dim) {
        std::vector<App::DocumentObject *> objs = dim->References.getValues();

        if(objs.size() > 0) {
            std::vector<App::DocumentObject *> objs = dim->References.getValues();
            // Attach the dimension to the first object's group
            for(std::vector<GIBase *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
                TechDraw::DrawView *viewObj = (*it)->getViewObject();
                if(strcmp(viewObj->getNameInDocument(), objs.at(0)->getNameInDocument()) == 0) {
                    return *it;
                }
            }
        }
    }
/* TODO: Pending splitting QGIViewCollection
    // Check if part of view collection
    for(std::vector<GIBase *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
        QGIViewCollection *grp = 0;
        grp = dynamic_cast<QGIViewCollection *>(*it);
        if(grp) {
            DrawViewCollection *collection = 0;
            collection = dynamic_cast<DrawViewCollection *>(grp->getViewObject());
            if(collection) {
                std::vector<App::DocumentObject *> objs = collection->Views.getValues();
                for( std::vector<App::DocumentObject *>::iterator it = objs.begin(); it != objs.end(); ++it) {
                    if(strcmp(myView->getNameInDocument(), (*it)->getNameInDocument()) == 0)

                        return grp;
                }
            }
        }
     }
*/
    // Not found a parent
    return 0;
}

GIPage::~GIPage()
{
    m_page->deregisterGi(this);
}

void GIPage::saveSvg(QString filename)
{
    const QString docName( QString::fromUtf8(m_page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(m_page->getNameInDocument()) );
    QString svgDescription = tr("Drawing page: ") + pageName +
                             tr(" exported from FreeCAD document: ") + docName;

    //Base::Console().Message("TRACE - saveSVG - page width: %d height: %d\n",width,height);    //A4 297x210
    QSvgGenerator svgGen;
    svgGen.setFileName(filename);
    svgGen.setSize(QSize((int) m_page->getPageWidth(), (int)m_page->getPageHeight()));
    svgGen.setViewBox(QRect(0, 0, m_page->getPageWidth(), m_page->getPageHeight()));
    //TODO: Exported Svg file is not quite right. <svg width="301.752mm" height="213.36mm" viewBox="0 0 297 210"... A4: 297x210
    //      Page too small (A4 vs Letter? margins?)
    //TODO: text in Qt is in mm (actually scene units).  text in SVG is points(?). fontsize in export file is too small by 1/2.835.
    //      resize all textItem before export?
    //      postprocess generated file to mult all font-size attrib by 2.835 to get pts?
    //      duplicate all textItems and only show the appropriate one for screen/print vs export?
    svgGen.setResolution(25.4000508);    // mm/inch??  docs say this is DPI
    //svgGen.setResolution(600);    // resulting page is ~12.5x9mm
    //svgGen.setResolution(96);     // page is ~78x55mm
    svgGen.setTitle(QObject::tr("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    // Might need this in some cases?  Gets called at the end of toggleEdit, so
    // at least the GUI case seems to not need it.
    //scene()->update();

    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p);
    p.end();
}

int GIPage::attachView(App::DocumentObject *obj)
{
    qDebug() << "in GIPage::attachView";
    assert(0);  // TODO: Implement this
    return -1;
}

void GIPage::attachTemplate(TechDraw::DrawTemplate *obj)
{
    assert(0);  // TODO: Implement this
}

