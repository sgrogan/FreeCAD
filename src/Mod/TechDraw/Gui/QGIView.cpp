/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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
    #include <QAction>
    #include <QGraphicsScene>
    #include <QGraphicsSceneHoverEvent>
    #include <QGraphicsSceneMouseEvent>
    #include <QMouseEvent>
    #include <QPainter>
    #include <QStyleOptionGraphicsItem>
#endif

#include "App/Document.h"
#include "Base/Console.h"
#include "Gui/Selection.h"
#include "Gui/Command.h"

#include "QGCustomBorder.h"
#include "QGCustomLabel.h"

#include "QGIView.h"

using namespace TechDrawGui;

QGIView::QGIView() :
    borderVisible(true)
{
    m_decorPen.setStyle(Qt::DashLine);
    m_decorPen.setWidth(0); // 0 => 1px "cosmetic pen"

    m_label = new QGCustomLabel();
    addToGroup(m_label);
    m_label->setFont(m_font);

    m_border = new QGCustomBorder();
    addToGroup(m_border);

    isVisible(true);
}

void QGIView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if(locked) {
        event->ignore();
    } else {
      QGraphicsItem::mousePressEvent(event);
    }
}

void QGIView::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    if(!borderVisible){
         m_label->hide();
         m_border->hide();
    }
    GIBase::paint(painter, &myOption, widget);
}

void QGIView::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(!locked && isSelected()) {
        if (!isInnerView()) {
            double tempX = x(),
                   tempY = getY();
            getViewObject()->X.setValue(tempX);
            getViewObject()->Y.setValue(tempY);
        } else {
            getViewObject()->X.setValue(x());
            getViewObject()->Y.setValue(getYInClip(y()));
        }
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIView::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // TODO don't like this but only solution at the minute
    if (isSelected()) {
        m_colCurrent = m_colSel;
    } else {
        m_colCurrent = m_colPre;
        //if(shape().contains(event->pos())) {                     // TODO don't like this for determining preselect
        //    m_colCurrent = m_colPre;
        //}
    }
    drawBorder();
    //update();
}

void QGIView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if(isSelected()) {
        m_colCurrent = m_colSel;
    } else {
        m_colCurrent = m_colNormal;
    }
    drawBorder();
    //update();
}

void QGIView::toggleCache(bool state)
{
    // TODO temp for devl. chaching was hiding problems WF
    setCacheMode((state)? NoCache : NoCache);
}

void QGIView::toggleBorder(bool state)
{
    borderVisible = state;
    drawBorder();
}

void QGIView::draw()
{
    if (isVisible()) {
        show();
    } else {
        hide();
    }
}

// TODO: Do we want to move the label drawing stuff up into GIBase?  No. It uses Gui/QGCustomXXX, so better here. -WF
void QGIView::drawBorder()
{
    if (!borderVisible) {
        return;
    }

    prepareGeometryChange();
    m_label->hide();
    m_border->hide();

    m_label->setDefaultTextColor(m_colCurrent);
    m_label->setFont(m_font);
    QString labelStr = QString::fromUtf8(getViewObject()->Label.getValue());
    m_label->setPlainText(labelStr);
    QRectF labelArea = m_label->boundingRect();
    double labelWidth = m_label->boundingRect().width();
    double labelHeight = m_label->boundingRect().height();

    m_border->hide();
    m_decorPen.setColor(m_colCurrent);
    m_border->setPen(m_decorPen);

    QRectF displayArea = customChildrenBoundingRect();
    double displayWidth = displayArea.width();
    double displayHeight = displayArea.height();

    double frameWidth = displayWidth;
    if (labelWidth > displayWidth) {
        frameWidth = labelWidth;
    }
    double frameHeight = labelHeight + displayHeight;
    QPointF displayCenter = displayArea.center();

    m_label->setX(displayCenter.x() - labelArea.width()/2.);
    m_label->setY(displayArea.bottom());

    QRectF frameArea = QRectF(displayCenter.x() - frameWidth/2.,
                              displayArea.top(),
                              frameWidth,
                              frameHeight);
    m_border->setRect(frameArea);
    m_border->setPos(0.,0.);

    m_label->show();
    m_border->show();
}


//! calculate rectangle border must contain
QRectF QGIView::customChildrenBoundingRect() {
    QList<QGraphicsItem*> children = childItems();
    int dimItemType = QGraphicsItem::UserType + 106;  // TODO: Magic number warning. make include file for custom types?
    int borderItemType = QGraphicsItem::UserType + 136;  // TODO: Magic number warning
    int labelItemType = QGraphicsItem::UserType + 135;  // TODO: Magic number warning
    QRectF result;
    for (QList<QGraphicsItem*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ( ((*it)->type() != dimItemType) &&
             ((*it)->type() != borderItemType) &&
             ((*it)->type() != labelItemType) ) {
            result = result.united((*it)->boundingRect());
        }
    }
    return result;
}

QVariant QGIView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            m_colCurrent = m_colSel;
        } else {
            m_colCurrent = m_colNormal;
        }
        drawBorder();
    }

    return GIBase::itemChange(change, value);
}
