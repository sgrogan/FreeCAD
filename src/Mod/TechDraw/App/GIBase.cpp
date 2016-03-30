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
    #include <QGraphicsScene> // TODO: This doesn't seem to be in the precompiled header?
#endif // #ifndef _PreComp_

#include "App/Application.h"
#include "App/Material.h"
#include "App/PropertyLinks.h"
#include "Base/Parameter.h"

#include "DrawView.h"
#include "DrawView.h"

#include "GIBase.h"

using namespace TechDraw;

GIBase::GIBase(const QPoint &pos, QGraphicsScene *scene)
    : QGraphicsItemGroup(),
      locked(false),
      m_innerView(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setAcceptHoverEvents(true);
    setPos(pos);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asQColor();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asQColor();

    m_colCurrent = m_colNormal;
    m_pen.setColor(m_colCurrent);

    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    m_font.setFamily(QString::fromStdString(fontName));
    m_font.setPointSize(5.0);     //scene units (mm), not points

    //Add object to scene
    scene->addItem(this);

    m_label = new QGraphicsTextItem();
    addToGroup(m_label);
    m_label->setFont(m_font);

    m_border = new QGraphicsRectItem();
    addToGroup(m_border);
}

const char * GIBase::getViewName() const
{
    return viewName.c_str();
}

void GIBase::setViewFeature(TechDraw::DrawView *obj)
{
    if(obj == 0)
        return;

    viewObj = obj;
    viewName = obj->getNameInDocument();

    // Set the QGIGroup initial position based on the DrawView
    float x = obj->X.getValue();
    float y = obj->Y.getValue();
    setPosition(x, y);
}

DrawView * GIBase::getViewObject() const
{
     return viewObj;
}

void GIBase::setPosition(qreal x, qreal y)
{
    if (!isInnerView()) {
        setPos(x, -y);            //position on page
    } else {
        setPos(x, getYInClip(y)); //position in Clip
    }
}

double GIBase::getYInClip(double y)
{
    /* TODO: Just removing this for now - should be easier when ViewClip has been split
    QGCustomClip* parentClip = dynamic_cast<QGCustomClip*>(parentItem());
    if (parentClip) {
        QGIViewClip* parentView = dynamic_cast<QGIViewClip*>(parentClip->parentItem());
        DrawViewClip* parentFeat = dynamic_cast<DrawViewClip*>(parentView->getViewObject());
        double newY = parentFeat->Height.getValue() - y;
        return newY;
    } else {
        Base::Console().Log("Logic Error - getYInClip called for child (%s) not in Clip\n",getViewName());
    }
    */
    return 0.0;
}

void GIBase::alignTo(QGraphicsItem *item, const QString &alignment)
{
    alignHash.clear();
    alignHash.insert(alignment, item);
}

void GIBase::updateView(bool update)
{
    if (update ||
        getViewObject()->X.isTouched() ||
        getViewObject()->Y.isTouched()) {
        double featX = getViewObject()->X.getValue();
        double featY = getViewObject()->Y.getValue();
        setPosition(featX,featY);
    }

    if (update ||
        getViewObject()->Rotation.isTouched()) {
        //NOTE: QPainterPaths have to be rotated individually. This transform handles everything else.
        double rot = getViewObject()->Rotation.getValue();
        QPointF centre = boundingRect().center();
        setTransform(QTransform().translate(centre.x(), centre.y()).rotate(-rot).translate(-centre.x(), -centre.y()));
    }

    if (update)
        QGraphicsItem::update();
}

void GIBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsItemGroup::paint(painter, option, widget);
}

QVariant GIBase::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();

        if(locked){
            newPos.setX(pos().x());
            newPos.setY(pos().y());
        }

        // TODO  find a better data structure for this
        if(alignHash.size() == 1) {
            QGraphicsItem*item = alignHash.begin().value();
            QString alignMode   = alignHash.begin().key();

            if(alignMode == QString::fromAscii("Vertical")) {
                newPos.setX(item->pos().x());
            } else if(alignMode == QString::fromAscii("Horizontal")) {
                newPos.setY(item->pos().y());
            } else if(alignMode == QString::fromAscii("45slash")) {
                double dist = ( (newPos.x() - item->pos().x()) +
                                (item->pos().y() - newPos.y()) ) / 2.0;

                newPos.setX( item->pos().x() + dist);
                newPos.setY( item->pos().y() - dist );
            } else if(alignMode == QString::fromAscii("45backslash")) {
                double dist = ( (newPos.x() - item->pos().x()) +
                                (newPos.y() - item->pos().y()) ) / 2.0;

                newPos.setX( item->pos().x() + dist);
                newPos.setY( item->pos().y() + dist );
            }
        }
        return newPos;
    }

    return QGraphicsItemGroup::itemChange(change, value);
}

