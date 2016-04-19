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
    #include <QSvgGenerator>
    #include <QPainter>
#endif // #ifndef _PreComp_

#include "Base/Console.h"

#include "../DrawParametricTemplate.h"
#include "../DrawProjGroup.h"
#include "../DrawSVGTemplate.h"
#include "../DrawViewSection.h"
#include "../DrawViewPart.h"
#include "../DrawViewCollection.h"
#include "../DrawViewDimension.h"
#include "../DrawViewAnnotation.h"
#include "../DrawViewSymbol.h"
#include "../DrawViewClip.h"

#include "GIBase.h"
#include "GICollection.h"
#include "GIDrawingTemplate.h"
#include "GISVGTemplate.h"

#include "GIPage.h"

#include <QDebug>   // TODO: Remove this
using namespace TechDraw;

GIPage::GIPage(DrawPage *page, QObject *sceneParent) :
    m_page(page),
    m_scene(new QGraphicsScene(sceneParent))
{
    assert(m_page);
    m_page->registerGi(this);
}


GIPage::~GIPage()
{
    m_page->deregisterGi(this);
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
        attachTemplate(dynamic_cast<DrawTemplate *>(obj));
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
    for(const auto it : views) {
        TechDraw::DrawView *fview = it->getViewObject();
        if(fview && strcmp(obj->getNameInDocument(), fview->getNameInDocument()) == 0)
            return it;
    }

    return nullptr;
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
    // Check if part of view collection
    for(std::vector<GIBase *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
        GICollection *grp = 0;
        grp = dynamic_cast<GICollection *>(*it);
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

    // Not found a parent
    return 0;
}


void GIPage::saveSvg(QString filename)
{
    assert(m_scene);

    const QString docName( QString::fromUtf8(m_page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(m_page->getNameInDocument()) );
    QString svgDescription = QObject::tr("Drawing page: ") + pageName +
                             QObject::tr(" exported from FreeCAD document: ") + docName;

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
    m_scene->render(&p);
    p.end();
}


int GIPage::attachView(App::DocumentObject *obj)
{
    GIBase *view(nullptr);

    auto typeId(obj->getTypeId());

    if(typeId.isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId()) ) {
        qDebug() << "Should add Section";
//        view = addViewSection( dynamic_cast<TechDraw::DrawViewSection *>(obj) );
    } else if (typeId.isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        qDebug() << "Should add Part";
//        view = addViewPart( dynamic_cast<TechDraw::DrawViewPart *>(obj) );
    } else if (typeId.isDerivedFrom(TechDraw::DrawProjGroup::getClassTypeId()) ) {
        qDebug() << "Should add ProjectionGroup";
//        view = addProjectionGroup( dynamic_cast<TechDraw::DrawProjGroup *>(obj) );
    } else if (typeId.isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId()) ) {
        qDebug() << "Should add Collection";
//        view = addDrawView( dynamic_cast<TechDraw::DrawViewCollection *>(obj) );
    } else if(typeId.isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()) ) {
        qDebug() << "Should add Dimension";
//        view = addViewDimension( dynamic_cast<TechDraw::DrawViewDimension *>(obj) );
    } else if(typeId.isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId()) ) {
        qDebug() << "Should add Annotation";
//        view = addDrawViewAnnotation( dynamic_cast<TechDraw::DrawViewAnnotation *>(obj) );
    } else if(typeId.isDerivedFrom(TechDraw::DrawViewSymbol::getClassTypeId()) ) {
        qDebug() << "Should add Symbol";
//        view = addDrawViewSymbol( dynamic_cast<TechDraw::DrawViewSymbol *>(obj) );
    } else if(typeId.isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId()) ) {
        qDebug() << "Should add Clip";
//        view = addDrawViewClip( dynamic_cast<TechDraw::DrawViewClip *>(obj) );
    } else {
        Base::Console().Log("Logic Error - Unknown view type in GIPage::attachView()");
        assert(0);
    }

    if(!view)
        return -1;
    else
        return views.size();
}


TechDraw::GITemplate* GIPage::getTemplate() const
{
    return pageTemplate.get();
}


void GIPage::removeTemplate()
{
    if(pageTemplate) {
        m_scene->removeItem(pageTemplate.get());
        pageTemplate.reset();
    }
}


void GIPage::attachTemplate(TechDraw::DrawTemplate *obj)
{
    removeTemplate();

    if(obj->isDerivedFrom(DrawParametricTemplate::getClassTypeId())) {
        pageTemplate.reset( new GIDrawingTemplate );
    } else if(obj->isDerivedFrom(DrawSVGTemplate::getClassTypeId())) {
        pageTemplate.reset( new GISVGTemplate );
    }

    pageTemplate->setTemplate(obj);
    pageTemplate->updateView();
    m_scene->addItem(pageTemplate.get());

    setSceneLimits();
}


void GIPage::setSceneLimits()
{
    if (!pageTemplate) {
        return;
    }

    auto drawTemplate(pageTemplate->getTemplate());

    auto width( drawTemplate->Width.getValue() );
    auto height( drawTemplate->Height.getValue() );

    //the +/- 1 is because of the way the template is define???
    m_scene->setSceneRect(QRectF(-1., -height, width+1., height));
}

